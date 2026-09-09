#include <algorithm>

#include <catch2/catch_test_macros.hpp>

#include "libslic3r/boss/surface/absorption/SparseInfillAbsorption.hpp"
#include "libslic3r/Flow.hpp"
#include "libslic3r/Point.hpp"

using namespace Slic3r;

namespace {

ExPolygon rectangle(const Point &origin, const coord_t width, const coord_t height)
{
    return {
        origin,
        origin + Point{width, 0},
        origin + Point{width, height},
        origin + Point{0, height},
    };
}

SurfaceFill make_fill(SurfaceType type, float density, double spacing_mm)
{
    SurfaceFillParams params;
    params.density = density;
    params.spacing = spacing_mm;
    params.flow    = Flow(float(spacing_mm), float(spacing_mm), float(spacing_mm) * 4.f);
    SurfaceFill fill(params);
    fill.surface = Surface(type, ExPolygon());
    return fill;
}

} // namespace

TEST_CASE("A small stInternalSolid hole surrounded by sparse infill is absorbed", "[boss][surface]")
{
    // Establishes the sparse threshold. At this test's density/spacing the sparse
    // rectangle also gets absorbed into the solid fill by absorb_small_sparse_pockets(),
    // but that has no bearing on this test's assertions: the hole is already removed by
    // remove_small_internal_solid_holes(), a separate, earlier step. This test does not
    // isolate absorb_small_sparse_pockets() -- see the dedicated notch test below for that.
    SurfaceFill sparse = make_fill(stInternal, 15.f, 0.45);
    sparse.expolygons  = {rectangle({0, 0}, scaled(1.0), scaled(1.0))};

    SurfaceFill solid  = make_fill(stInternalSolid, 100.f, 0.45);
    ExPolygon   body   = rectangle({0, 0}, scaled(20.0), scaled(20.0));
    Polygon     hole   = rectangle({scaled(9.0), scaled(9.0)}, scaled(0.3), scaled(0.3)).contour;
    hole.reverse();
    body.holes.push_back(hole);
    solid.expolygons = {body};

    const ExPolygons total_fill_boundary{rectangle({-scaled(1.0), -scaled(1.0)}, scaled(22.0), scaled(22.0))};

    std::vector<SurfaceFill> surface_fills{sparse, solid};
    Boss::SparseInfillAbsorption::absorb(surface_fills, total_fill_boundary);

    REQUIRE(surface_fills[1].expolygons.size() == 1);
    REQUIRE(surface_fills[1].expolygons.front().holes.empty());
}

TEST_CASE("A hole extending past the fill boundary is kept as a real model feature", "[boss][surface]")
{
    SurfaceFill sparse = make_fill(stInternal, 15.f, 0.45);
    sparse.expolygons  = {rectangle({0, 0}, scaled(1.0), scaled(1.0))};

    SurfaceFill solid = make_fill(stInternalSolid, 100.f, 0.45);
    ExPolygon   body  = rectangle({0, 0}, scaled(20.0), scaled(20.0));
    // A hole that pokes out through the (narrower) fill boundary -- most
    // of its area lies outside total_fill_boundary, so it must survive.
    Polygon hole = rectangle({scaled(14.0), scaled(5.0)}, scaled(3.0), scaled(3.0)).contour;
    hole.reverse();
    body.holes.push_back(hole);
    solid.expolygons = {body};

    const ExPolygons total_fill_boundary{rectangle({0, 0}, scaled(15.0), scaled(20.0))};

    std::vector<SurfaceFill> surface_fills{sparse, solid};
    Boss::SparseInfillAbsorption::absorb(surface_fills, total_fill_boundary);

    REQUIRE(surface_fills[1].expolygons.size() == 1);
    REQUIRE(surface_fills[1].expolygons.front().holes.size() == 1);
}

TEST_CASE("A small sparse pocket sitting in a stSolidOverBridge notch is absorbed via closing_ex",
          "[boss][surface]")
{
    // A single stSolidOverBridge fragment shaped like a "C": a 20x20 square with a
    // 2mm-deep, 4mm-tall notch bitten out of the right edge. Because this is one
    // expolygon (not several disjoint fragments), the grow/union/shrink merge step
    // that closes gaps between fragments never runs (it needs more than one
    // expolygon) -- so absorb_small_sparse_pockets()'s own closing_ex() call is the
    // only mechanism in absorb() that can recognize the notch pocket as covered.
    SurfaceFill bridge = make_fill(stSolidOverBridge, 100.f, 0.45);
    ExPolygon   notched{
        Point{scaled(0.0), scaled(0.0)},
        Point{scaled(0.0), scaled(20.0)},
        Point{scaled(20.0), scaled(20.0)},
        Point{scaled(20.0), scaled(12.0)},
        Point{scaled(18.0), scaled(12.0)},
        Point{scaled(18.0), scaled(8.0)},
        Point{scaled(20.0), scaled(8.0)},
        Point{scaled(20.0), scaled(0.0)},
    };
    bridge.expolygons = {notched};

    SurfaceFill sparse = make_fill(stInternal, 15.f, 0.45);
    sparse.expolygons  = {rectangle({scaled(18.0), scaled(8.0)}, scaled(2.0), scaled(4.0))};

    const ExPolygons total_fill_boundary{rectangle({-scaled(1.0), -scaled(1.0)}, scaled(23.0), scaled(23.0))};

    std::vector<SurfaceFill> surface_fills{bridge, sparse};
    Boss::SparseInfillAbsorption::absorb(surface_fills, total_fill_boundary);

    REQUIRE(surface_fills[1].expolygons.empty());
}

TEST_CASE("Two stSolidOverBridge fragments split by bridge angle consolidate into one entry", "[boss][surface]")
{
    SurfaceFill sob_a      = make_fill(stSolidOverBridge, 100.f, 0.45);
    sob_a.params.bridge_angle = 0.f;
    sob_a.expolygons          = {rectangle({0, 0}, scaled(5.0), scaled(5.0))};

    SurfaceFill sob_b         = make_fill(stSolidOverBridge, 100.f, 0.45);
    sob_b.params.bridge_angle = 1.57f;
    sob_b.expolygons          = {rectangle({scaled(10.0), 0}, scaled(5.0), scaled(5.0))};

    const ExPolygons total_fill_boundary{rectangle({-scaled(1.0), -scaled(1.0)}, scaled(20.0), scaled(20.0))};

    std::vector<SurfaceFill> surface_fills{sob_a, sob_b};
    Boss::SparseInfillAbsorption::absorb(surface_fills, total_fill_boundary);

    const auto non_empty =
        std::count_if(surface_fills.begin(), surface_fills.end(), [](const SurfaceFill &sf) { return !sf.expolygons.empty(); });
    REQUIRE(non_empty == 1);
}
