#include <memory>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "boss/features/external-first-holes/ExternalFirstHolesFeature.hpp"
#include "libslic3r/Arachne/PerimeterOrder.hpp"
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

TEST_CASE("External-first holes sets holes_external_first and propagates min size",
          "[boss][perimeter]")
{
    TestConfig config;
    config.print.items.opt("external_perimeters_first_holes_min_size").set(30.0);
    config.tool[0].overrides.set("external_perimeters_first_holes", true);
    PrintRegionConfigView view = build_region_config_view(config);

    Slic3r::Boss::PerimeterPolicyContext ctx;
    ctx.layer_id = 3;
    ctx.extruder_id = 0;
    ctx.config = &view;
    Slic3r::Boss::OrderingPolicy policy;
    Slic3r::Boss::ExternalFirstHolesFeature::apply_ordering(policy, ctx);
    REQUIRE(policy.holes_external_first == true);
    REQUIRE(policy.min_hole_perimeter_length == 30.0);
}

TEST_CASE("External-first holes respects disabled-first-layers", "[boss][perimeter]")
{
    TestConfig config;
    config.print.items.opt("external_perimeters_first_disabled_first_layers").set(2);
    config.tool[0].overrides.set("external_perimeters_first_holes", true);
    PrintRegionConfigView view = build_region_config_view(config);

    Slic3r::Boss::PerimeterPolicyContext ctx;
    ctx.layer_id = 0;
    ctx.extruder_id = 0;
    ctx.config = &view;
    Slic3r::Boss::OrderingPolicy policy;
    policy.contours_external_first = true;
    Slic3r::Boss::ExternalFirstHolesFeature::apply_ordering(policy, ctx);
    REQUIRE(policy.holes_external_first == false);
    REQUIRE(policy.contours_external_first == false);
}

namespace {

using Slic3r::Arachne::ExtrusionLine;
using Slic3r::Arachne::Perimeter;
using Slic3r::Arachne::Perimeters;
using Slic3r::Arachne::PerimeterOrder::ordered_perimeter_extrusions;
using Slic3r::Point;
using Slic3r::scaled;

// A closed, counter-clockwise square loop -- Arachne's convention makes a
// CCW loop a hole (ExtrusionLine::is_contour() requires clockwise).
ExtrusionLine make_hole_ring(coord_t x0, coord_t y0, coord_t size, size_t inset_idx)
{
    ExtrusionLine line(inset_idx, /*is_odd=*/false, /*is_closed=*/true);
    const coord_t width = scaled(0.4);
    line.emplace_back(Point(x0, y0), width, coord_t(inset_idx));
    line.emplace_back(Point(x0 + size, y0), width, coord_t(inset_idx));
    line.emplace_back(Point(x0 + size, y0 + size), width, coord_t(inset_idx));
    line.emplace_back(Point(x0, y0 + size), width, coord_t(inset_idx));
    line.emplace_back(Point(x0, y0), width, coord_t(inset_idx)); // close the loop
    return line;
}

} // namespace

TEST_CASE("Arachne ordered_perimeter_extrusions reverses only holes below the min size",
          "[boss][perimeter]")
{
    // Below-threshold hole: side 1000 -> perimeter ~4000, well under 30mm.
    ExtrusionLine small_outer = make_hole_ring(0, 0, 1000, 0);
    ExtrusionLine small_inner = make_hole_ring(300, 300, 400, 1);

    // Above-threshold hole: side 20mm -> perimeter ~80mm, well over 30mm.
    // Placed far from the small hole so their bounding boxes never overlap.
    const coord_t large_side = scaled(20.0);
    const coord_t large_offset = scaled(1000.0);
    ExtrusionLine large_outer = make_hole_ring(large_offset, large_offset, large_side, 0);
    ExtrusionLine large_inner = make_hole_ring(
        large_offset + scaled(2.0), large_offset + scaled(2.0), large_side - scaled(4.0), 1
    );

    REQUIRE_FALSE(small_outer.is_contour());
    REQUIRE_FALSE(large_outer.is_contour());

    Perimeters perimeters = {Perimeter{small_outer, small_inner, large_outer, large_inner}};

    Slic3r::Boss::OrderingPolicy ordering;
    ordering.contours_external_first = true;
    ordering.holes_external_first    = true;
    ordering.min_hole_perimeter_length = 30.0;

    Slic3r::Arachne::PerimeterOrder::PerimeterExtrusions result =
        ordered_perimeter_extrusions(perimeters, ordering);
    REQUIRE(result.size() == 4);

    const coord_t midpoint = large_offset / 2;
    // Finds the loop belonging to the small hole (x < midpoint) or the large
    // hole (x >= midpoint) with the given inset index.
    auto index_of_inset = [&](bool small_hole, size_t inset_idx) {
        for (size_t i = 0; i < result.size(); ++i) {
            const bool is_small = result[i].extrusion.junctions.front().p.x() < midpoint;
            if (result[i].extrusion.inset_idx == inset_idx && is_small == small_hole)
                return i;
        }
        FAIL("extrusion not found");
        return size_t(0);
    };

    // Below the min size: the hole's own reversal falls back to the native
    // "external last" order, even though holes_external_first is enabled.
    REQUIRE(index_of_inset(true, 1) < index_of_inset(true, 0));

    // Above the min size: holes_external_first keeps the native "external
    // first" order in place.
    REQUIRE(index_of_inset(false, 0) < index_of_inset(false, 1));
}
