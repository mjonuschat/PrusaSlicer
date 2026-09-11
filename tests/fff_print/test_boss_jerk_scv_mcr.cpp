#include <memory>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "boss/features/jerk-scv-mcr/JerkScvMcrFeature.hpp"
#include "boss/foundation/ExtrusionContext.hpp"
#include "boss/foundation/MotionDynamics.hpp"
#include "libslic3r/ConfigViews.hpp"
#include "libslic3r/ExtrudeConfig.hpp"
#include "libslic3r/ExtrusionRole.hpp"
#include "Slic3r/Domain/FullConfigFDM.hpp"
#include "test_data.hpp"

using Slic3r::ExtrusionRole;
using Slic3r::PrintRegionConfigView;
using Slic3r::Biz::Slicing::ExtrudeConfig;
using Slic3r::Boss::ExtrusionContext;
using Slic3r::Boss::JerkScvMcrFeature;
using Slic3r::Boss::MotionDynamics;
using Slic3r::Domain::FullConfigFDM;
using Slic3r::Domain::ObjectSettings;
using Slic3r::Domain::PartialObjectConfigFDM;
using Slic3r::Domain::PartialVolumeConfigFDM;
using Slic3r::Domain::VolumeSettings;
using Slic3r::Test::TestConfig;

namespace {

PrintRegionConfigView build_region_config_view(const TestConfig& config)
{
    const auto full_config{std::make_shared<const FullConfigFDM>(config.get_full_config())};
    PartialObjectConfigFDM object_config{ObjectSettings{}, full_config->hw_config()};
    PartialVolumeConfigFDM volume_config{VolumeSettings{}, full_config->hw_config()};

    PrintRegionConfigView view{
        full_config,
        std::make_shared<const PartialObjectConfigFDM>(std::move(object_config)),
        {std::make_shared<const PartialVolumeConfigFDM>(std::move(volume_config))}
    };
    view.finalize();
    return view;
}

} // namespace

TEST_CASE("JerkScvMcrFeature::before_extrusion: first_layer wins over bridge", "[boss][motion]")
{
    TestConfig config;
    // MCR rides on the acceleration cascade's own per-role gate: setting
    // <role>_acceleration is what makes that role's branch win, independent of
    // the MCR value itself.
    config.print.items.opt("first_layer_acceleration").set(1000.0);
    config.print.items.opt("first_layer_minimum_cruise_ratio").set(0.5);
    config.print.items.opt("first_layer_jerk").set(3000);
    config.print.items.opt("bridge_acceleration").set(500.0);
    config.print.items.opt("bridge_minimum_cruise_ratio").set(0.2);
    config.print.items.opt("bridge_jerk").set(1000);
    config.print.items.opt("default_jerk").set(1);
    PrintRegionConfigView view = build_region_config_view(config);
    ExtrudeConfig extrude_config{view};

    ExtrusionContext ctx{ExtrusionRole::BridgeInfill, /*is_first_layer=*/true, false, 0};
    ctx.extrude_config = &extrude_config;

    std::optional<MotionDynamics> result = JerkScvMcrFeature::before_extrusion(ctx);
    REQUIRE(result.has_value());
    CHECK(result->minimum_cruise_ratio == 0.5);
    CHECK(result->jerk == 3000);
}

TEST_CASE("JerkScvMcrFeature::before_extrusion: bridge wins over infill", "[boss][motion]")
{
    TestConfig config;
    config.print.items.opt("bridge_acceleration").set(500.0);
    config.print.items.opt("bridge_minimum_cruise_ratio").set(0.2);
    config.print.items.opt("bridge_jerk").set(1000);
    config.print.items.opt("infill_acceleration").set(800.0);
    config.print.items.opt("infill_minimum_cruise_ratio").set(0.7);
    config.print.items.opt("infill_jerk").set(4000);
    config.print.items.opt("default_jerk").set(1);
    PrintRegionConfigView view = build_region_config_view(config);
    ExtrudeConfig extrude_config{view};

    ExtrusionContext ctx{ExtrusionRole::BridgeInfill, false, false, 0};
    ctx.extrude_config = &extrude_config;

    std::optional<MotionDynamics> result = JerkScvMcrFeature::before_extrusion(ctx);
    REQUIRE(result.has_value());
    CHECK(result->minimum_cruise_ratio == 0.2);
    CHECK(result->jerk == 1000);
}

TEST_CASE(
    "JerkScvMcrFeature::before_extrusion: falls through to default when the role's own "
    "acceleration is unset",
    "[boss][motion]"
)
{
    TestConfig config;
    // infill_acceleration stays at its default (0), so the accel-gated MCR
    // resolution skips the infill branch entirely and falls to default's MCR,
    // even though infill_minimum_cruise_ratio is itself set.
    config.print.items.opt("infill_minimum_cruise_ratio").set(0.7);
    config.print.items.opt("infill_jerk").set(4000);
    config.print.items.opt("default_minimum_cruise_ratio").set(0.3);
    config.print.items.opt("default_jerk").set(2000);
    PrintRegionConfigView view = build_region_config_view(config);
    ExtrudeConfig extrude_config{view};

    ExtrusionContext ctx{ExtrusionRole::InternalInfill, false, false, 0};
    ctx.extrude_config = &extrude_config;

    std::optional<MotionDynamics> result = JerkScvMcrFeature::before_extrusion(ctx);
    REQUIRE(result.has_value());
    CHECK(result->minimum_cruise_ratio == 0.3);
    // Jerk resolves independently of MCR/acceleration: infill_jerk > 0 on its own
    // is enough for the infill branch to win the jerk cascade.
    CHECK(result->jerk == 4000);
}

TEST_CASE(
    "JerkScvMcrFeature::before_extrusion: nullopt when nothing is configured",
    "[boss][motion]"
)
{
    TestConfig config;
    PrintRegionConfigView view = build_region_config_view(config);
    ExtrudeConfig extrude_config{view};

    ExtrusionContext ctx{ExtrusionRole::InternalInfill, false, false, 0};
    ctx.extrude_config = &extrude_config;

    CHECK_FALSE(JerkScvMcrFeature::before_extrusion(ctx).has_value());
}

TEST_CASE(
    "JerkScvMcrFeature::before_extrusion: first_layer wins over first_layer_over_raft",
    "[boss][motion]"
)
{
    TestConfig config;
    config.print.items.opt("first_layer_acceleration").set(1000.0);
    config.print.items.opt("first_layer_minimum_cruise_ratio").set(0.4);
    config.print.items.opt("first_layer_jerk").set(2500);
    config.print.items.opt("first_layer_acceleration_over_raft").set(900.0);
    config.print.items.opt("first_layer_minimum_cruise_ratio_over_raft").set(0.6);
    config.print.items.opt("first_layer_jerk_over_raft").set(3500);
    config.print.items.opt("default_jerk").set(1);
    PrintRegionConfigView view = build_region_config_view(config);
    ExtrudeConfig extrude_config{view};

    // is_first_layer and is_object_layer_over_raft are not mutually exclusive in
    // GCode.cpp -- the first object layer over a raft is also the first layer --
    // and the 2.9.x cascade checks on_first_layer() before object_layer_over_raft(),
    // so first_layer takes priority when both are true.
    ExtrusionContext ctx{ExtrusionRole::InternalInfill, true, true, 0};
    ctx.extrude_config = &extrude_config;

    std::optional<MotionDynamics> result = JerkScvMcrFeature::before_extrusion(ctx);
    REQUIRE(result.has_value());
    CHECK(result->minimum_cruise_ratio == 0.4);
    CHECK(result->jerk == 2500);
}

TEST_CASE(
    "JerkScvMcrFeature::before_extrusion: first_layer_over_raft resolves when first_layer is unset",
    "[boss][motion]"
)
{
    TestConfig config;
    config.print.items.opt("first_layer_acceleration_over_raft").set(900.0);
    config.print.items.opt("first_layer_minimum_cruise_ratio_over_raft").set(0.6);
    config.print.items.opt("first_layer_jerk_over_raft").set(3500);
    config.print.items.opt("default_jerk").set(1);
    PrintRegionConfigView view = build_region_config_view(config);
    ExtrudeConfig extrude_config{view};

    ExtrusionContext ctx{ExtrusionRole::InternalInfill, false, true, 0};
    ctx.extrude_config = &extrude_config;

    std::optional<MotionDynamics> result = JerkScvMcrFeature::before_extrusion(ctx);
    REQUIRE(result.has_value());
    CHECK(result->minimum_cruise_ratio == 0.6);
    CHECK(result->jerk == 3500);
}

TEST_CASE(
    "JerkScvMcrFeature::before_extrusion: a role's MCR applies even when that role's own MCR "
    "value is zero",
    "[boss][motion]"
)
{
    // 2.9.x never gates MCR on its own value -- once a role wins the
    // acceleration-driven predicate, its MCR is used as-is, even if that is 0.
    TestConfig config;
    config.print.items.opt("bridge_acceleration").set(500.0);
    config.print.items.opt("bridge_jerk").set(1000);
    config.print.items.opt("default_minimum_cruise_ratio").set(0.9);
    config.print.items.opt("default_jerk").set(1);
    PrintRegionConfigView view = build_region_config_view(config);
    ExtrudeConfig extrude_config{view};

    ExtrusionContext ctx{ExtrusionRole::BridgeInfill, false, false, 0};
    ctx.extrude_config = &extrude_config;

    std::optional<MotionDynamics> result = JerkScvMcrFeature::before_extrusion(ctx);
    REQUIRE(result.has_value());
    CHECK(result->minimum_cruise_ratio == 0.0);
    CHECK(result->jerk == 1000);
}

TEST_CASE(
    "JerkScvMcrFeature::before_extrusion: jerk cascade can pick a different role than MCR",
    "[boss][motion]"
)
{
    // The role a specific extrusion resolves for MCR (accel-gated) and the role it
    // resolves for jerk (independently gated on jerk itself) can differ.
    TestConfig config;
    config.print.items.opt("bridge_acceleration").set(500.0);
    config.print.items.opt("bridge_minimum_cruise_ratio").set(0.2);
    // bridge_jerk stays at its default (0): the jerk cascade skips bridge and
    // falls through to default_jerk instead.
    config.print.items.opt("default_jerk").set(4200);
    PrintRegionConfigView view = build_region_config_view(config);
    ExtrudeConfig extrude_config{view};

    ExtrusionContext ctx{ExtrusionRole::BridgeInfill, false, false, 0};
    ctx.extrude_config = &extrude_config;

    std::optional<MotionDynamics> result = JerkScvMcrFeature::before_extrusion(ctx);
    REQUIRE(result.has_value());
    CHECK(result->minimum_cruise_ratio == 0.2);
    CHECK(result->jerk == 4200);
}

TEST_CASE(
    "JerkScvMcrFeature::before_extrusion: travel resolves travel roles, not the extrusion cascade",
    "[boss][motion]"
)
{
    TestConfig config;
    config.print.items.opt("travel_minimum_cruise_ratio").set(0.1);
    config.print.items.opt("travel_jerk").set(6000);
    config.print.items.opt("travel_short_distance_minimum_cruise_ratio").set(0.9);
    config.print.items.opt("travel_short_distance_jerk").set(8000);
    config.print.items.opt("default_jerk").set(1);
    PrintRegionConfigView view = build_region_config_view(config);
    ExtrudeConfig extrude_config{view};

    ExtrusionContext travel_ctx{ExtrusionRole::Perimeter, false, false, 0};
    travel_ctx.extrude_config = &extrude_config;
    travel_ctx.is_travel      = true;

    std::optional<MotionDynamics> travel_result = JerkScvMcrFeature::before_extrusion(travel_ctx);
    REQUIRE(travel_result.has_value());
    CHECK(travel_result->minimum_cruise_ratio == 0.1);
    CHECK(travel_result->jerk == 6000);

    ExtrusionContext short_travel_ctx         = travel_ctx;
    short_travel_ctx.is_short_distance_travel = true;

    std::optional<MotionDynamics> short_travel_result =
        JerkScvMcrFeature::before_extrusion(short_travel_ctx);
    REQUIRE(short_travel_result.has_value());
    CHECK(short_travel_result->minimum_cruise_ratio == 0.9);
    CHECK(short_travel_result->jerk == 8000);
}

TEST_CASE(
    "JerkScvMcrFeature::before_extrusion: travel jerk needs default_jerk set too",
    "[boss][motion]"
)
{
    // 2.9.x gates travel jerk on default_jerk > 0 && travel_jerk > 0 -- MCR has no
    // such gate.
    TestConfig config;
    config.print.items.opt("travel_minimum_cruise_ratio").set(0.4);
    config.print.items.opt("travel_jerk").set(6000);
    PrintRegionConfigView view = build_region_config_view(config);
    ExtrudeConfig extrude_config{view};

    ExtrusionContext ctx{ExtrusionRole::Perimeter, false, false, 0};
    ctx.extrude_config = &extrude_config;
    ctx.is_travel      = true;

    std::optional<MotionDynamics> result = JerkScvMcrFeature::before_extrusion(ctx);
    REQUIRE(result.has_value());
    CHECK(result->minimum_cruise_ratio == 0.4);
    CHECK(result->jerk == 0);
}

TEST_CASE(
    "JerkScvMcrFeature::before_extrusion: nullopt without an extrude_config pointer",
    "[boss][motion]"
)
{
    ExtrusionContext ctx{ExtrusionRole::InternalInfill, false, false, 0};
    CHECK_FALSE(JerkScvMcrFeature::before_extrusion(ctx).has_value());
}

TEST_CASE("jerk-scv-mcr: perimeter jerk reaches the sliced G-code as M205", "[boss][motion]")
{
    TestConfig config;
    config.printer.items.opt("gcode_flavor").set(Slic3r::Domain::GCodeFlavor::gcfMarlinFirmware);
    config.print.items.opt("perimeter_jerk").set(3000);
    config.print.items.opt("default_jerk").set(1500);

    const std::string gcode = Slic3r::Test::slice({Slic3r::Test::TestMesh::cube_20x20x20}, config);
    REQUIRE_FALSE(gcode.empty());
    CHECK(Slic3r::Test::contains(gcode, "M205 X3000 Y3000"));
}

TEST_CASE("jerk-scv-mcr: travel jerk reaches the sliced G-code as M205", "[boss][motion]")
{
    TestConfig config;
    config.printer.items.opt("gcode_flavor").set(Slic3r::Domain::GCodeFlavor::gcfMarlinFirmware);
    config.print.items.opt("travel_jerk").set(4000);
    config.print.items.opt("default_jerk").set(1500);

    const std::string gcode = Slic3r::Test::slice({Slic3r::Test::TestMesh::cube_20x20x20}, config);
    REQUIRE_FALSE(gcode.empty());
    CHECK(Slic3r::Test::contains(gcode, "M205 X4000 Y4000"));
}
