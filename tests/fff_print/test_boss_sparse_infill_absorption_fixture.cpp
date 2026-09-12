// Proves the real Fill.cpp call site -- not the absorb() function in
// isolation -- removes a sub-threshold hole. The hole is injected directly
// into a real LayerRegion's fill_surfaces() before the single group_fills()
// call this test observes, so the one production code path
// (group_fills() -> Slic3r::Boss::SparseInfillAbsorption::absorb()) is what
// performs the removal, not a second, test-driven call to absorb() on
// group_fills()'s own output (which cannot distinguish a correctly wired
// call site from a missing one, since group_fills() already ran absorb()
// once by the time such a second call would see its result).
#include <catch2/catch_test_macros.hpp>

#include "Slic3r/Biz/Algorithms/Polygon.hpp"
#include "libslic3r/ClipperUtils.hpp"
#include "libslic3r/Fill/Fill.hpp"
#include "libslic3r/Layer.hpp"
#include "libslic3r/LayerRegion.hpp"
#include "libslic3r/Point.hpp"
#include "libslic3r/Print.hpp"
#include "libslic3r/Surface.hpp"
#include "libslic3r/SurfaceCollection.hpp"

#include "test_data.hpp"

using namespace Slic3r;
using namespace Slic3r::Test;

TEST_CASE("A stInternalSolid hole injected before group_fills() is absorbed by its live call site", "[boss][fill][fixture]")
{
    TestConfig config;
    config.print.items.opt("skirts").set(0);
    config.print.items.opt("fill_density").set(Domain::Percentage{15.0});
    config.print.items.opt("top_solid_layers").set(3);
    config.print.items.opt("bottom_solid_layers").set(3);

    Print print;
    Slic3r::Test::init_and_process_print({TestMesh::cube_20x20x20}, print, config);

    // Find a layer with both a sparse (stInternal) surface -- needed for
    // absorb()'s sparse threshold to be non-zero -- and a non-empty
    // stInternalSolid surface to inject into. The surface may already carry
    // holes from real classification (unaffected by absorption, which only
    // runs inside group_fills() on its derived output, never on
    // fill_surfaces() itself) -- the assertion below targets the injected
    // hole specifically by size, not by an absolute hole count.
    Surface *target_surface = nullptr;
    Layer   *target_layer   = nullptr;
    for (Layer *layer : print.get_object(0)->layers()) {
        bool has_sparse = false;
        for (const LayerRegion *layerm : layer->regions())
            for (const Surface &surface : layerm->fill_surfaces().surfaces)
                if (surface.surface_type == stInternal && !surface.empty())
                    has_sparse = true;
        if (!has_sparse)
            continue;

        for (LayerRegion *layerm : layer->regions()) {
            // fill_surfaces() only exposes a const accessor; layerm itself
            // is a genuinely non-const LayerRegion*, so stripping the
            // const off this reference to mutate its public `surfaces`
            // member is well-defined, not undefined behavior.
            auto &fill_surfaces = const_cast<SurfaceCollection &>(layerm->fill_surfaces());
            for (Surface &surface : fill_surfaces.surfaces)
                if (surface.surface_type == stInternalSolid && !surface.empty()) {
                    target_surface = &surface;
                    break;
                }
            if (target_surface)
                break;
        }
        if (target_surface) {
            target_layer = layer;
            break;
        }
    }
    REQUIRE(target_surface != nullptr);

    const size_t holes_before_injection = target_surface->expolygon.holes.size();

    // Inject a hole far below any plausible sparse-fill area threshold,
    // fully inside the region and not shared with any other fill.
    const BoundingBox box      = Biz::Algorithms::Polygon::get_extents(target_surface->expolygon.contour);
    const Point        centroid = (box.min + box.max) / 2;
    const coord_t      half_side = scaled(0.1);
    Polygon hole{
        centroid + Point{-half_side, -half_side},
        centroid + Point{half_side, -half_side},
        centroid + Point{half_side, half_side},
        centroid + Point{-half_side, half_side},
    };
    hole.reverse();
    target_surface->expolygon.holes.push_back(hole);
    REQUIRE(target_surface->expolygon.holes.size() == holes_before_injection + 1);

    // The one real call: group_fills() re-derives SurfaceFillParams from
    // the mutated fill_surfaces() and, on the production path, calls
    // Slic3r::Boss::SparseInfillAbsorption::absorb() on the result itself.
    const std::vector<SurfaceFill> surface_fills = group_fills(*target_layer);

    // A hole with the injected hole's area (0.2mm x 0.2mm) is orders of
    // magnitude below any real, load-bearing feature this test's config
    // could produce -- if one survives absorption, the injected hole was
    // not removed.
    const double tiny_hole_area_bound = std::abs(hole.area()) * 10.0;
    bool         tiny_hole_survived   = false;
    for (const SurfaceFill &sf : surface_fills)
        if (sf.surface.surface_type == stInternalSolid)
            for (const ExPolygon &ep : sf.expolygons)
                for (const Polygon &remaining_hole : ep.holes)
                    if (std::abs(remaining_hole.area()) < tiny_hole_area_bound)
                        tiny_hole_survived = true;
    REQUIRE_FALSE(tiny_hole_survived);
}
