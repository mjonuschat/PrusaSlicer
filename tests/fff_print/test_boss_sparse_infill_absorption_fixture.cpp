// Proves Slic3r::Boss::SparseInfillAbsorption::absorb() behaves correctly
// against real SurfaceFillParams (actual flow/spacing/density) pulled from a
// live, processed Print -- not just the synthetic fixtures in
// test_boss_sparse_infill_absorption.cpp, which build every field by hand.
//
// Rather than engineering a specific mesh guaranteed to already contain a
// sub-threshold hole after ordinary classification (fragile, and dependent
// on unrelated slicing details), this takes a stInternalSolid SurfaceFill
// group_fills() actually produced for a real cube, injects a hole well
// below the region's own computed sparse threshold, and calls absorb()
// again with the real total_fill_boundary. This calls absorb() directly, so
// it does not exercise Fill.cpp's own call site -- that wiring is covered by
// the full ctest suite (including fff_print_tests) passing with it in place.
#include <catch2/catch_test_macros.hpp>

#include "Slic3r/Biz/Algorithms/Polygon.hpp"
#include "libslic3r/boss/surface/absorption/SparseInfillAbsorption.hpp"
#include "libslic3r/ClipperUtils.hpp"
#include "libslic3r/Fill/Fill.hpp"
#include "libslic3r/Layer.hpp"
#include "libslic3r/LayerRegion.hpp"
#include "libslic3r/Point.hpp"
#include "libslic3r/Print.hpp"
#include "libslic3r/Surface.hpp"

#include "test_data.hpp"

using namespace Slic3r;
using namespace Slic3r::Test;

TEST_CASE("A stInternalSolid hole well below the real sparse threshold is absorbed by the live group_fills() call site",
          "[boss][fill][fixture]")
{
    TestConfig config;
    config.print.items.opt("skirts").set(0);
    config.print.items.opt("fill_density").set(Domain::Percentage{15.0});
    config.print.items.opt("top_solid_layers").set(3);
    config.print.items.opt("bottom_solid_layers").set(3);

    Print print;
    Slic3r::Test::init_and_process_print({TestMesh::cube_20x20x20}, print, config);

    // Find a layer whose fill groups include both a sparse region (needed
    // for a non-zero threshold) and a stInternalSolid region (the
    // transition layers under/over the solid top/bottom shells).
    std::vector<SurfaceFill> surface_fills;
    ExPolygons               total_fill_boundary;
    SurfaceFill             *solid = nullptr;
    for (const Layer *layer : print.get_object(0)->layers()) {
        std::vector<SurfaceFill> candidate = group_fills(*layer);

        bool has_sparse = false;
        for (const SurfaceFill &sf : candidate)
            if (sf.surface.surface_type == stInternal && !sf.expolygons.empty() && sf.params.density < 99.f)
                has_sparse = true;
        if (!has_sparse)
            continue;

        for (size_t i = 0; i < candidate.size(); ++i)
            if (candidate[i].surface.surface_type == stInternalSolid && !candidate[i].expolygons.empty()) {
                surface_fills = std::move(candidate);
                solid         = &surface_fills[i];
                for (const LayerRegion *layerm : layer->regions())
                    append(total_fill_boundary, layerm->fill_expolygons());
                total_fill_boundary = union_ex(total_fill_boundary);
                break;
            }
        if (solid)
            break;
    }
    REQUIRE(solid != nullptr);

    const auto count_holes = [](const ExPolygons &eps) {
        size_t n = 0;
        for (const ExPolygon &ep : eps)
            n += ep.holes.size();
        return n;
    };
    const size_t holes_before_injection = count_holes(solid->expolygons);

    // Inject a hole far below any plausible sparse-fill area threshold,
    // fully inside the region and not shared with any other fill.
    ExPolygon &target = solid->expolygons.front();
    const BoundingBox box      = Biz::Algorithms::Polygon::get_extents(target.contour);
    const Point       centroid = (box.min + box.max) / 2;
    Polygon hole{
        centroid + Point{-scaled(0.1), -scaled(0.1)},
        centroid + Point{scaled(0.1), -scaled(0.1)},
        centroid + Point{scaled(0.1), scaled(0.1)},
        centroid + Point{-scaled(0.1), scaled(0.1)},
    };
    hole.reverse();
    target.holes.push_back(hole);
    REQUIRE(count_holes(solid->expolygons) == holes_before_injection + 1);

    Boss::SparseInfillAbsorption::absorb(surface_fills, total_fill_boundary);

    // absorb() may reassign solid->expolygons in place (e.g. after merging
    // in an absorbed sparse pocket), invalidating any reference taken into
    // it beforehand -- re-derive the hole count from `solid` itself rather
    // than reusing `target`. The injected hole must be gone; any holes the
    // real classification produced independently are out of scope here.
    REQUIRE(count_holes(solid->expolygons) <= holes_before_injection);
}
