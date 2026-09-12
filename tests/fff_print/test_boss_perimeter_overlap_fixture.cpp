#include <algorithm>

#include <catch2/catch_test_macros.hpp>

#include "libslic3r/ExtrusionEntityCollection.hpp"
#include "libslic3r/Flow.hpp"
#include "libslic3r/PerimeterGenerator.hpp"
#include "libslic3r/Point.hpp"
#include "libslic3r/Surface.hpp"
#include "libslic3r/SurfaceCollection.hpp"
#include "libslic3r/libslic3r.h"
#include "Slic3r/Biz/Algorithms/Polygon.hpp"

#include "test_data.hpp"

using namespace Slic3r;
using Test::TestConfig;
using Domain::FullConfigFDM;
using Domain::FloatOrPercentage;
using Domain::ObjectSettings;
using Domain::Percentage;
using Domain::VolumeSettings;

namespace {

// Distance between two axis-aligned loop bounding boxes' matching edges,
// i.e. how far the inner loop is inset from the outer one.
coord_t inset_distance(const BoundingBox &outer, const BoundingBox &inner)
{
    const coord_t outer_width = outer.max.x() - outer.min.x();
    const coord_t inner_width = inner.max.x() - inner.min.x();
    return (outer_width - inner_width) / 2;
}

// Runs PerimeterGenerator::process_classic on a plain square with the given
// perimeter_perimeter_overlap and returns the loops sorted outermost-first.
std::vector<Polygon> generate_loops(const FloatOrPercentage &perimeter_perimeter_overlap)
{
    TestConfig config;
    config.print.items.opt("perimeters").set(3);
    config.print.items.opt("perimeter_extrusion_width").set(FloatOrPercentage{0.6});
    config.print.items.opt("external_perimeter_extrusion_width").set(FloatOrPercentage{0.6});
    config.print.items.opt("layer_height").set(0.3);
    config.print.items.opt("perimeter_perimeter_overlap").set(perimeter_perimeter_overlap);

    const auto full_config{std::make_shared<const FullConfigFDM>(config.get_full_config())};
    Domain::PartialObjectConfigFDM object_config{ObjectSettings{}, full_config->hw_config()};
    Domain::PartialVolumeConfigFDM volume_config{VolumeSettings{}, full_config->hw_config()};
    PrintRegionConfigView region_config_view{
        full_config,
        std::make_shared<const Domain::PartialObjectConfigFDM>(std::move(object_config)),
        {std::make_shared<const Domain::PartialVolumeConfigFDM>(std::move(volume_config))}
    };
    region_config_view.finalize();

    ExPolygons expolygons{ExPolygon{Slic3r::Biz::Algorithms::Polygon::scaled(
        {{0, 0}, {100, 0}, {100, 100}, {0, 100}})}};

    SurfaceCollection slices;
    slices.append(expolygons, stInternal);

    ExtrusionEntityCollection loops;
    ExtrusionEntityCollection gap_fill;
    ExPolygons               fill_expolygons;
    const Flow                flow(0.6f, 0.3f, 0.4f);
    PerimeterRegions          perimeter_regions;
    PerimeterGenerator::Parameters perimeter_generator_params(
        0.3, // layer height
        -1,  // layer ID
        flow, flow, flow, flow,
        region_config_view,
        perimeter_regions,
        false); // spiral_vase
    Polygons lower_layer_polygons_cache;
    for (const Surface &surface : slices)
        PerimeterGenerator::process_classic(
            perimeter_generator_params, surface, nullptr, nullptr,
            lower_layer_polygons_cache, loops, gap_fill, fill_expolygons);

    loops = loops.flatten();
    std::vector<Polygon> polygons;
    for (const ExtrusionEntity *entity : loops.entities)
        polygons.push_back(dynamic_cast<const ExtrusionLoop *>(entity)->polygon());

    std::sort(polygons.begin(), polygons.end(), [](const Polygon &a, const Polygon &b) {
        const BoundingBox bb_a = get_extents(a.points);
        const BoundingBox bb_b = get_extents(b.points);
        return (bb_a.max.x() - bb_a.min.x()) > (bb_b.max.x() - bb_b.min.x());
    });
    return polygons;
}

} // namespace

TEST_CASE("BOSS perimeter_perimeter_overlap shrinks inner-wall spacing", "[boss][perimeter]")
{
    // Baseline: the default 10.73% overlap, which reproduces the plain
    // (pre-feature) spacing formula.
    const std::vector<Polygon> baseline = generate_loops(FloatOrPercentage{Percentage{10.73}});
    // Increased overlap: config.ini-equivalent value from the task brief.
    const std::vector<Polygon> increased = generate_loops(FloatOrPercentage{Percentage{20.}});

    REQUIRE(baseline.size() == 3);
    REQUIRE(increased.size() == 3);

    // Spacing between the two internal perimeters (the innermost pair),
    // which is what perimeter_perimeter_overlap controls.
    const coord_t baseline_spacing  = inset_distance(get_extents(baseline[1].points), get_extents(baseline[2].points));
    const coord_t increased_spacing = inset_distance(get_extents(increased[1].points), get_extents(increased[2].points));

    REQUIRE(increased_spacing < baseline_spacing);
}
