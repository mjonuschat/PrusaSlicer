#include <catch2/catch_test_macros.hpp>

#include "libslic3r/Fill/Fill.hpp"
#include "libslic3r/Fill/FillBase.hpp"
#include "libslic3r/Layer.hpp"
#include "libslic3r/Print.hpp"
#include "libslic3r/Surface.hpp"

#include "test_data.hpp"

using namespace Slic3r;
using namespace Slic3r::Test;

namespace {

Polylines fill_first_surface(Print &print, SurfaceType surface_type, Domain::InfillPattern *out_pattern = nullptr)
{
    for (const Layer *layer : print.get_object(0)->layers())
        for (SurfaceFill &fill : group_fills(*layer))
            if (fill.surface.surface_type == surface_type && !fill.expolygons.empty()) {
                if (out_pattern != nullptr)
                    *out_pattern = fill.params.pattern;
                fill.surface.expolygon = fill.expolygons.front();
                std::unique_ptr<Fill> filler(Fill::new_from_type(fill.params.pattern));
                filler->angle = fill.params.angle;
                filler->spacing = fill.params.spacing;
                FillParams fill_params;
                fill_params.density = float(0.01 * fill.params.density);
                return filler->fill_surface(&fill.surface, fill_params);
            }
    return {};
}

void slice_with_solid_fill_pattern(Print &print, Domain::InfillPattern solid_fill_pattern)
{
    TestConfig config;
    config.print.items.opt("skirts").set(0);
    config.print.items.opt("perimeters").set(1);
    config.print.items.opt("fill_density").set(Domain::Percentage{15.0});
    config.print.items.opt("top_solid_layers").set(3);
    config.print.items.opt("bottom_solid_layers").set(3);
    config.print.items.opt("solid_fill_pattern").set(solid_fill_pattern);

    Slic3r::Test::init_and_process_print({TestMesh::cube_20x20x20}, print, config);
}

} // namespace

TEST_CASE("Internal solid infill uses the configured solid_fill_pattern, not ipEnsuring", "[boss][fill][fixture]")
{
    Print print;
    slice_with_solid_fill_pattern(print, Domain::InfillPattern::ipConcentric);

    Domain::InfillPattern pattern{};
    Polylines paths = fill_first_surface(print, stInternalSolid, &pattern);

    REQUIRE_FALSE(paths.empty());
    REQUIRE(pattern == Domain::InfillPattern::ipConcentric);
    REQUIRE(pattern != Domain::InfillPattern::ipEnsuring);
    REQUIRE(pattern != Domain::InfillPattern::ipMonotonic);
}

TEST_CASE("Internal solid infill geometry changes with solid_fill_pattern", "[boss][fill][fixture]")
{
    Print print_concentric;
    slice_with_solid_fill_pattern(print_concentric, Domain::InfillPattern::ipConcentric);
    Print print_rectilinear;
    slice_with_solid_fill_pattern(print_rectilinear, Domain::InfillPattern::ipRectilinear);

    Polylines concentric = fill_first_surface(print_concentric, stInternalSolid);
    Polylines rectilinear = fill_first_surface(print_rectilinear, stInternalSolid);

    REQUIRE_FALSE(concentric.empty());
    REQUIRE_FALSE(rectilinear.empty());
    REQUIRE(concentric != rectilinear);
}

TEST_CASE("Top and bottom solid infill are unaffected by solid_fill_pattern", "[boss][fill][fixture]")
{
    Print print_concentric;
    slice_with_solid_fill_pattern(print_concentric, Domain::InfillPattern::ipConcentric);
    Print print_rectilinear;
    slice_with_solid_fill_pattern(print_rectilinear, Domain::InfillPattern::ipRectilinear);

    Polylines top_concentric = fill_first_surface(print_concentric, stTop);
    Polylines top_rectilinear = fill_first_surface(print_rectilinear, stTop);
    Polylines bottom_concentric = fill_first_surface(print_concentric, stBottom);
    Polylines bottom_rectilinear = fill_first_surface(print_rectilinear, stBottom);

    REQUIRE_FALSE(top_concentric.empty());
    REQUIRE_FALSE(bottom_concentric.empty());
    REQUIRE(top_concentric == top_rectilinear);
    REQUIRE(bottom_concentric == bottom_rectilinear);
}

TEST_CASE("Bridge infill is unaffected by solid_fill_pattern", "[boss][fill][fixture]")
{
    TestConfig config;
    config.print.items.opt("top_solid_layers").set(0);
    config.print.items.opt("solid_fill_pattern").set(Domain::InfillPattern::ipConcentric);

    Print print;
    Slic3r::Test::init_and_process_print({TestMesh::bridge}, print, config);

    bool found_bridge = false;
    for (const Layer *layer : print.get_object(0)->layers())
        for (SurfaceFill &fill : group_fills(*layer))
            if (fill.surface.is_bridge() && !fill.expolygons.empty()) {
                found_bridge = true;
                REQUIRE(fill.params.pattern != Domain::InfillPattern::ipConcentric);
            }

    REQUIRE(found_bridge);
}
