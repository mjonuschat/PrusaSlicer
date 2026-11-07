#include <catch2/catch_test_macros.hpp>

#include "Slic3r/Biz/Algorithms/TriangleMesh.hpp"
#include "libslic3r/Fill/Fill.hpp"
#include "libslic3r/Layer.hpp"
#include "libslic3r/Print.hpp"
#include "libslic3r/Surface.hpp"

#include "test_data.hpp"

using namespace Slic3r;
using namespace Slic3r::Test;

TEST_CASE("A genuinely narrow internal solid region reaches classify() and ends up with pattern ipEnsuring",
          "[boss][fill][fixture]")
{
    TestConfig config;
    config.print.items.opt("skirts").set(0);
    config.print.items.opt("perimeters").set(1);
    config.print.items.opt("fill_density").set(Domain::Percentage{20.0});
    config.print.items.opt("detect_narrow_solid_infill").set(true);
    config.print.items.opt("detect_narrow_solid_infill_threshold").set(10.0);

    Print print;
    Slic3r::Test::init_and_process_print({TestMesh::cube_2x20x10}, print, config);

    bool found_internal_solid_ensuring = false;
    for (const Layer *layer : print.get_object(0)->layers())
        for (const SurfaceFill &fill : group_fills(*layer))
            if (fill.surface.surface_type == stInternalSolid && fill.params.pattern == Domain::InfillPattern::ipEnsuring)
                found_internal_solid_ensuring = true;

    REQUIRE(found_internal_solid_ensuring);
}

namespace {

// A narrow bar whose top-layer solid surface both erodes away under classify() and has a
// small enough area to pass the top/bottom skip's area guard.
bool top_surface_is_dropped(bool detect_narrow_solid_infill)
{
    TestConfig config;
    config.print.items.opt("skirts").set(0);
    config.print.items.opt("perimeters").set(1);
    config.print.items.opt("fill_density").set(Domain::Percentage{20.0});
    config.print.items.opt("detect_narrow_solid_infill").set(detect_narrow_solid_infill);
    config.print.items.opt("detect_narrow_solid_infill_threshold").set(10.0);

    Print print;
    Domain::TriangleMesh mesh{Slic3r::Biz::Algorithms::TriangleMesh::make_cube(1.4, 2.0, 3.0)};
    Slic3r::Test::init_and_process_print({mesh}, print, config);

    const std::vector<Layer *> &layers = print.get_object(0)->layers();
    REQUIRE(! layers.empty());
    for (const SurfaceFill &fill : group_fills(*layers.back()))
        if (fill.surface.surface_type == stTop)
            return false;
    return true;
}

} // namespace

TEST_CASE("A narrow top sliver is dropped when detect_narrow_solid_infill is on", "[boss][fill][fixture]")
{
    REQUIRE(top_surface_is_dropped(true));
}

TEST_CASE("The narrow top sliver skip does not fire when detect_narrow_solid_infill is off",
          "[boss][fill][fixture]")
{
    REQUIRE_FALSE(top_surface_is_dropped(false));
}
