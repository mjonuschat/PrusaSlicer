#include <algorithm>
#include <cmath>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "libslic3r/Fill/boss/crosshatch/FillCrossHatch.hpp"
#include "libslic3r/Fill/FillBase.hpp"
#include "libslic3r/Point.hpp"
#include "libslic3r/Surface.hpp"
#include "libslic3r/libslic3r.h"

using namespace Slic3r;

TEST_CASE("BOSS CrossHatch Fill subtype is instantiable", "[boss][fill]")
{
    Slic3r::FillCrossHatch fill;
    REQUIRE(fill.clone() != nullptr);
}

TEST_CASE("BOSS CrossHatch fill produces genuinely crosshatched geometry", "[boss][fill]")
{
    Points square{
        Point(coord_t(scale_(0.0)), coord_t(scale_(0.0))),
        Point(coord_t(scale_(20.0)), coord_t(scale_(0.0))),
        Point(coord_t(scale_(20.0)), coord_t(scale_(20.0))),
        Point(coord_t(scale_(0.0)), coord_t(scale_(20.0))),
    };
    Surface surface(stInternal, ExPolygon(square));

    FillCrossHatch fill;
    fill.angle   = 0.f;
    fill.spacing = 0.4;
    // z=0.1mm with this spacing lands mid-way through a transform layer
    // (see generate_infill_layers), so the fill produces the zig-zag
    // transform pattern, not the plain parallel-line repeat pattern.
    fill.z = 0.1;

    FillParams params;
    params.density          = 1.f;
    // Skip connect_infill's perimeter-hugging travel moves so the
    // asserted geometry comes only from the pattern generator itself.
    params.anchor_length_max = 0.f;

    Polylines polylines = fill.fill_surface(&surface, params);
    REQUIRE(!polylines.empty());

    // A zig-zag transform-layer polyline has more than two points, unlike
    // a straight repeat-layer line.
    bool has_zigzag_polyline = std::any_of(polylines.begin(), polylines.end(),
                                            [](const Polyline &pl) { return pl.points.size() > 2; });
    REQUIRE(has_zigzag_polyline);

    // Collect consecutive-segment directions and confirm at least two of
    // them are not collinear -- the geometric signature of a crosshatch
    // (alternating-direction) pattern, as opposed to a set of parallel
    // rectilinear lines.
    std::vector<Vec2d> directions;
    for (const Polyline &pl : polylines) {
        for (size_t i = 1; i < pl.points.size(); ++i) {
            Vec2d d = (pl.points[i] - pl.points[i - 1]).cast<double>();
            if (d.norm() > SCALED_EPSILON)
                directions.push_back(d.normalized());
        }
    }
    REQUIRE(directions.size() > 1);

    bool found_non_collinear_pair = false;
    for (size_t i = 0; i < directions.size() && !found_non_collinear_pair; ++i)
        for (size_t j = i + 1; j < directions.size(); ++j)
            if (std::abs(directions[i].dot(directions[j])) < 0.9) {
                found_non_collinear_pair = true;
                break;
            }
    REQUIRE(found_non_collinear_pair);
}
