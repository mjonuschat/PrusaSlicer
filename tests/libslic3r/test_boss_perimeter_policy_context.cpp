#include <memory>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "libslic3r/ConfigViews.hpp"
#include "libslic3r/ExtrusionEntityCollection.hpp"
#include "libslic3r/Flow.hpp"
#include "libslic3r/PerimeterGenerator.hpp"
#include "libslic3r/boss/perimeter/PerimeterPolicyContext.hpp"
#include "Slic3r/Domain/FullConfigFDM.hpp"
#include "fff_print/test_data.hpp"

using Slic3r::Flow;
using Slic3r::PerimeterRegions;
using Slic3r::PrintRegionConfigView;
using Slic3r::Test::TestConfig;
using Slic3r::Domain::FullConfigFDM;
using Slic3r::Domain::ObjectSettings;
using Slic3r::Domain::VolumeSettings;
using Slic3r::Domain::PartialObjectConfigFDM;
using Slic3r::Domain::PartialVolumeConfigFDM;

namespace {

PrintRegionConfigView build_region_config_view(const TestConfig &config)
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

TEST_CASE("make_perimeter_policy_context populates every field", "[boss][perimeter]")
{
    TestConfig config;
    config.print.items.opt("fill_density").set(Slic3r::Domain::Percentage{42.0});
    PrintRegionConfigView view = build_region_config_view(config);

    const Flow             flow(1., 1., 1.);
    const PerimeterRegions perimeter_regions;
    Slic3r::PerimeterGenerator::Parameters params(
        1.,    // layer height
        7,     // layer ID
        flow, flow, flow, flow,
        view,
        perimeter_regions,
        true); // spiral_vase

    const Slic3r::Boss::PerimeterPolicyContext ctx = Slic3r::Boss::make_perimeter_policy_context(params, 0);

    REQUIRE(ctx.layer_id == 7);
    REQUIRE(ctx.is_first_layer == false);
    REQUIRE(ctx.spiral_vase == true);
    REQUIRE(ctx.fill_density == 42.0);
    REQUIRE(ctx.extruder_id == 0);
    REQUIRE(ctx.config == &view);
}

TEST_CASE("make_perimeter_policy_context reports the first layer", "[boss][perimeter]")
{
    TestConfig config;
    PrintRegionConfigView view = build_region_config_view(config);

    const Flow             flow(1., 1., 1.);
    const PerimeterRegions perimeter_regions;
    Slic3r::PerimeterGenerator::Parameters params(
        1.,    // layer height
        0,     // layer ID
        flow, flow, flow, flow,
        view,
        perimeter_regions,
        false); // spiral_vase

    const Slic3r::Boss::PerimeterPolicyContext ctx = Slic3r::Boss::make_perimeter_policy_context(params, 0);

    REQUIRE(ctx.is_first_layer == true);
}
