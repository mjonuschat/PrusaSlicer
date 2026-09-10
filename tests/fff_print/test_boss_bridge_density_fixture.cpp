#include <algorithm>
#include <numeric>
#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "libslic3r/ExtrusionEntity.hpp"
#include "libslic3r/ExtrusionEntityCollection.hpp"
#include "libslic3r/Layer.hpp"
#include "libslic3r/LayerRegion.hpp"
#include "libslic3r/Print.hpp"

#include "test_data.hpp"

using namespace Slic3r;
using namespace Slic3r::Test;

namespace {

void collect_bridge_infill_polylines(const ExtrusionEntity &entity, Domain::Polylines &dst)
{
    if (const auto *collection = dynamic_cast<const ExtrusionEntityCollection *>(&entity)) {
        for (const ExtrusionEntity *child : collection->entities)
            collect_bridge_infill_polylines(*child, dst);
    } else if (const auto *path = dynamic_cast<const ExtrusionPath *>(&entity)) {
        if (path->role() == ExtrusionRole::BridgeInfill)
            dst.push_back(path->polyline);
    }
}

// The test bridge spans in X (angle 0 per the "Bridging integration" scenario in
// test_bridges.cpp), so adjacent fill lines are distinguished by their Y position.
// Returns the mean gap between consecutive distinct line positions, in mm.
double mean_bridge_line_spacing(const Print &print)
{
    Domain::Polylines polylines;
    for (const Layer *layer : print.get_object(0)->layers())
        for (size_t region_id = 0; region_id < layer->region_count(); ++region_id)
            collect_bridge_infill_polylines(layer->get_region(region_id)->fills(), polylines);

    // A rectilinear filler connects consecutive lines into one zigzagging polyline,
    // so every vertex (not just each polyline's first point) must be sampled to see
    // every line's Y position.
    std::vector<double> ys;
    for (const Domain::Polyline &polyline : polylines)
        for (const Domain::Point &point : polyline.points)
            ys.push_back(unscale<double>(point.y()));

    REQUIRE(ys.size() > 1);
    std::sort(ys.begin(), ys.end());

    // Cluster line positions that are within a tenth of a millimeter -- a single
    // fill line can be split into several polylines (anchors, clipped segments).
    std::vector<double> distinct_ys{ys.front()};
    for (double y : ys)
        if (y - distinct_ys.back() > 0.1)
            distinct_ys.push_back(y);

    REQUIRE(distinct_ys.size() > 1);
    std::vector<double> gaps;
    for (size_t i = 1; i < distinct_ys.size(); ++i)
        gaps.push_back(distinct_ys[i] - distinct_ys[i - 1]);
    return std::accumulate(gaps.begin(), gaps.end(), 0.) / double(gaps.size());
}

double slice_mean_bridge_line_spacing(double bridge_density_percent)
{
    TestConfig config;
    config.print.items.opt("top_solid_layers").set(0); // prevents solid infill from covering the bridge
    if (bridge_density_percent != 100.)
        config.print.items.opt("bridge_density").set(Domain::Percentage{bridge_density_percent});

    Print print;
    Slic3r::Test::init_and_process_print({TestMesh::bridge}, print, config);
    return mean_bridge_line_spacing(print);
}

} // namespace

TEST_CASE("External bridge fill spacing matches the native bridge flow at 100% bridge_density", "[boss][bridge_density]")
{
    TestConfig config;
    config.print.items.opt("top_solid_layers").set(0);

    Print print;
    Slic3r::Test::init_and_process_print({TestMesh::bridge}, print, config);

    const double measured_spacing = mean_bridge_line_spacing(print);
    const double native_spacing = print.get_object(0)->layers().front()->get_region(0)->bridging_flow(FlowRole::frSolidInfill).spacing();

    CHECK(measured_spacing == Catch::Approx(native_spacing).epsilon(0.2));
}

TEST_CASE("External bridge fill spacing widens as bridge_density decreases", "[boss][bridge_density]")
{
    const double spacing_100 = slice_mean_bridge_line_spacing(100.);
    const double spacing_50 = slice_mean_bridge_line_spacing(50.);

    const double ratio = spacing_50 / spacing_100;
    CHECK(ratio > 1.6);
    CHECK(ratio < 2.4);
}
