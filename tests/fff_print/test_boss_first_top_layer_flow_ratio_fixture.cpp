#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "libslic3r/Flow.hpp"
#include "libslic3r/Layer.hpp"
#include "libslic3r/LayerRegion.hpp"
#include "libslic3r/Print.hpp"

#include "test_data.hpp"

using namespace Slic3r;
using namespace Slic3r::Test;

namespace {

double first_layer_perimeter_flow_height(double flow_ratio)
{
    TestConfig config;
    if (flow_ratio != 1.)
        config.print.items.opt("first_layer_flow_ratio").set(flow_ratio);

    Print print;
    Slic3r::Test::init_and_process_print({TestMesh::cube_20x20x20}, print, config);

    return print.get_object(0)->layers().front()->get_region(0)->flow(FlowRole::frPerimeter).height();
}

double top_solid_infill_flow_height(double flow_ratio)
{
    TestConfig config;
    if (flow_ratio != 1.)
        config.print.items.opt("top_layer_flow_ratio").set(flow_ratio);

    Print print;
    Slic3r::Test::init_and_process_print({TestMesh::cube_20x20x20}, print, config);

    return print.get_object(0)->layers().back()->get_region(0)->flow(FlowRole::frTopSolidInfill).height();
}

} // namespace

TEST_CASE("first_layer_flow_ratio scales the first layer's flow height", "[boss][flow]")
{
    const double baseline = first_layer_perimeter_flow_height(1.);
    const double scaled = first_layer_perimeter_flow_height(1.2);

    CHECK(scaled / baseline == Catch::Approx(1.2));
}

TEST_CASE("top_layer_flow_ratio scales the top solid infill flow height", "[boss][flow]")
{
    const double baseline = top_solid_infill_flow_height(1.);
    const double scaled = top_solid_infill_flow_height(1.2);

    CHECK(scaled / baseline == Catch::Approx(1.2));
}
