#include <catch2/catch_test_macros.hpp>

#include "libslic3r/ExtrusionEntityCollection.hpp"
#include "libslic3r/Layer.hpp"
#include "libslic3r/PerimeterGenerator.hpp"
#include "libslic3r/Print.hpp"
#include "libslic3r/SurfaceCollection.hpp"
#include "Slic3r/Biz/Algorithms/Polygon.hpp"

#include "test_data.hpp"

using namespace Slic3r;
using Test::TestConfig;
using Domain::FullConfigFDM;
using Domain::ObjectSettings;
using Domain::VolumeSettings;

namespace {

// Runs Classic perimeter generation on a rectangle with one hole and returns
// the flattened, emission-ordered extrusion loops -- same technique as
// "Perimeter nesting" in test_perimeters.cpp, so its already-verified
// "Rectangle with hole" nesting/order data can be trusted as a baseline.
ExtrusionEntityCollection generate_rectangle_with_hole_loops(const TestConfig &config)
{
    const auto full_config{std::make_shared<const FullConfigFDM>(config.get_full_config())};
    Domain::PartialObjectConfigFDM object_config{ObjectSettings{}, full_config->hw_config()};
    Domain::PartialVolumeConfigFDM volume_config{VolumeSettings{}, full_config->hw_config()};
    PrintRegionConfigView region_config_view{
        full_config,
        std::make_shared<const Domain::PartialObjectConfigFDM>(std::move(object_config)),
        {std::make_shared<const Domain::PartialVolumeConfigFDM>(std::move(volume_config))}
    };
    region_config_view.finalize();

    ExPolygon rectangle_with_hole{
        Biz::Algorithms::Polygon::scaled({{0, 0}, {100, 0}, {100, 100}, {0, 100}}),
        Biz::Algorithms::Polygon::scaled({{40, 40}, {40, 60}, {60, 60}, {60, 40}})
    };

    SurfaceCollection slices;
    slices.append({rectangle_with_hole}, stInternal);

    ExtrusionEntityCollection loops;
    ExtrusionEntityCollection gap_fill;
    ExPolygons                fill_expolygons;
    Flow                      flow(1., 1., 1.);
    PerimeterRegions          perimeter_regions;
    PerimeterGenerator::Parameters params(
        1., // layer height
        0,  // layer ID
        flow, flow, flow, flow,
        region_config_view,
        perimeter_regions,
        false); // spiral_vase
    Polygons lower_layer_polygons_cache;
    for (const Surface &surface : slices)
        PerimeterGenerator::process_classic(
            params, surface, nullptr, nullptr, lower_layer_polygons_cache, loops, gap_fill, fill_expolygons
        );

    return loops.flatten();
}

// The hole's own loops sit well inside the outer 100x100 square, close to the
// 40..60 cavity -- unlike the contour's loops, which hug the outer boundary.
bool belongs_to_hole(const ExtrusionEntity &entity)
{
    const Point p = entity.first_point();
    return p.x() > scale_(30.) && p.x() < scale_(70.) && p.y() > scale_(30.) && p.y() < scale_(70.);
}

} // namespace

SCENARIO("BOSS external-first-holes changes the Classic perimeter print order", "[boss][perimeter]")
{
    GIVEN("a rectangle with one hole, 2 perimeters, external perimeters first for contours enabled, "
          "no brim, and no minimum hole size gating") {
        TestConfig config{1};
        config.print.items.opt("perimeter_generator").set(Domain::PerimeterGeneratorType::Classic);
        config.print.items.opt("perimeters").set(2);
        config.print.items.opt("external_perimeters_first").set(true);
        config.print.items.opt("external_perimeters_first_holes_min_size").set(0.0);
        config.print.items.opt("external_perimeters_first_disabled_first_layers").set(0);
        // brim_width defaults to 5mm, which on layer 0 would force the brim
        // continuation reversal regardless of external_perimeters_first_holes
        // and mask the option this test is isolating.
        config.print.items.opt("brim_width").set(0.0);

        WHEN("external_perimeters_first_holes is enabled") {
            config.print.items.opt("external_perimeters_first_holes").set(true);
            const ExtrusionEntityCollection loops = generate_rectangle_with_hole_loops(config);

            std::vector<const ExtrusionEntity *> hole_entities;
            for (const ExtrusionEntity *entity : loops.entities)
                if (belongs_to_hole(*entity))
                    hole_entities.push_back(entity);

            THEN("the hole's external perimeter is emitted before its internal perimeter") {
                REQUIRE(hole_entities.size() == 2);
                REQUIRE(hole_entities.front()->role() == ExtrusionRole::ExternalPerimeter);
                REQUIRE(hole_entities.back()->role() != ExtrusionRole::ExternalPerimeter);
            }
        }

        WHEN("external_perimeters_first_holes is disabled") {
            config.print.items.opt("external_perimeters_first_holes").set(false);
            const ExtrusionEntityCollection loops = generate_rectangle_with_hole_loops(config);

            std::vector<const ExtrusionEntity *> hole_entities;
            for (const ExtrusionEntity *entity : loops.entities)
                if (belongs_to_hole(*entity))
                    hole_entities.push_back(entity);

            THEN("the hole's external perimeter is emitted after its internal perimeter") {
                REQUIRE(hole_entities.size() == 2);
                REQUIRE(hole_entities.front()->role() != ExtrusionRole::ExternalPerimeter);
                REQUIRE(hole_entities.back()->role() == ExtrusionRole::ExternalPerimeter);
            }
        }
    }
}
