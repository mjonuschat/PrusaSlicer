#include <memory>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "boss/features/alternate-extra-perimeter/AlternateExtraPerimeterFeature.hpp"
#include "libslic3r/ConfigViews.hpp"
#include "libslic3r/boss/perimeter/PerimeterPolicyContext.hpp"
#include "Slic3r/Domain/FullConfigFDM.hpp"
#include "fff_print/test_data.hpp"

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

TEST_CASE("Alternate extra perimeter adds one loop on odd layers with density and no spiral vase",
          "[boss][perimeter]")
{
    TestConfig config;
    config.print.items.opt("alternate_extra_perimeter").set(true);
    PrintRegionConfigView view = build_region_config_view(config);

    Slic3r::Boss::PerimeterPolicyContext ctx;
    ctx.layer_id = 1;
    ctx.fill_density = 20.0;
    ctx.spiral_vase = false;
    ctx.config = &view;
    REQUIRE(Slic3r::Boss::AlternateExtraPerimeterFeature::adjust_loop_count(2, ctx) == 3);
}

TEST_CASE("Alternate extra perimeter does nothing on even layers", "[boss][perimeter]")
{
    TestConfig config;
    config.print.items.opt("alternate_extra_perimeter").set(true);
    PrintRegionConfigView view = build_region_config_view(config);

    Slic3r::Boss::PerimeterPolicyContext ctx;
    ctx.layer_id = 2;
    ctx.fill_density = 20.0;
    ctx.config = &view;
    REQUIRE(Slic3r::Boss::AlternateExtraPerimeterFeature::adjust_loop_count(2, ctx) == 2);
}

TEST_CASE("Alternate extra perimeter does nothing in spiral vase mode", "[boss][perimeter]")
{
    TestConfig config;
    config.print.items.opt("alternate_extra_perimeter").set(true);
    PrintRegionConfigView view = build_region_config_view(config);

    Slic3r::Boss::PerimeterPolicyContext ctx;
    ctx.layer_id = 1;
    ctx.fill_density = 20.0;
    ctx.spiral_vase = true;
    ctx.config = &view;
    REQUIRE(Slic3r::Boss::AlternateExtraPerimeterFeature::adjust_loop_count(2, ctx) == 2);
}

TEST_CASE("Alternate extra perimeter does nothing at zero fill density", "[boss][perimeter]")
{
    TestConfig config;
    config.print.items.opt("alternate_extra_perimeter").set(true);
    PrintRegionConfigView view = build_region_config_view(config);

    Slic3r::Boss::PerimeterPolicyContext ctx;
    ctx.layer_id = 1;
    ctx.fill_density = 0.0;
    ctx.config = &view;
    REQUIRE(Slic3r::Boss::AlternateExtraPerimeterFeature::adjust_loop_count(2, ctx) == 2);
}

TEST_CASE("Alternate extra perimeter does nothing when its own config key is off",
          "[boss][perimeter]")
{
    // alternate_extra_perimeter defaults to false -- proves the enable
    // check now lives inside adjust_loop_count() itself, not at the
    // registry dispatch call site, so the additive fold always runs.
    TestConfig config;
    PrintRegionConfigView view = build_region_config_view(config);

    Slic3r::Boss::PerimeterPolicyContext ctx;
    ctx.layer_id = 1;
    ctx.fill_density = 20.0;
    ctx.spiral_vase = false;
    ctx.config = &view;
    REQUIRE(Slic3r::Boss::AlternateExtraPerimeterFeature::adjust_loop_count(2, ctx) == 2);
}
