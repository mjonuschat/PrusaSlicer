#include <cmath>
#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "libslic3r/Fill/FillPlanePath.hpp"
#include "libslic3r/Point.hpp"
#include "libslic3r/Surface.hpp"
#include "libslic3r/libslic3r.h"

using namespace Slic3r;

TEST_CASE("BOSS Flowsnake Fill subtype is instantiable", "[boss][fill]")
{
    Slic3r::FillFlowsnake fill;
    REQUIRE(fill.clone() != nullptr);
}

TEST_CASE("BOSS Flowsnake applies its density/overlap correction", "[boss][fill]")
{
    // flow_correction()/spacing_correction() are overridden as `protected` on
    // FillFlowsnake itself -- call through a base Fill& reference, whose
    // public virtual these override, rather than the concrete FillFlowsnake
    // type directly.
    Slic3r::FillFlowsnake concrete_fill;
    Slic3r::Fill& fill = concrete_fill;
    REQUIRE(fill.flow_correction() == Catch::Approx(0.85));
}

TEST_CASE("BOSS Flowsnake fill produces genuinely Gosper-curve geometry", "[boss][fill]")
{
    Points square{
        Point(coord_t(scale_(0.0)), coord_t(scale_(0.0))),
        Point(coord_t(scale_(20.0)), coord_t(scale_(0.0))),
        Point(coord_t(scale_(20.0)), coord_t(scale_(20.0))),
        Point(coord_t(scale_(0.0)), coord_t(scale_(20.0))),
    };
    Surface surface(stInternal, ExPolygon(square));

    FillFlowsnake fill;
    fill.angle   = 0.f;
    fill.spacing = 0.4;

    FillParams params;
    params.density = 1.f;

    Polylines polylines = fill.fill_surface(&surface, params);
    REQUIRE(!polylines.empty());

    // The Gosper curve walks in 6 hexagonal directions (multiples of 60
    // degrees), unlike Hilbert curve or Octagram Spiral, whose segments
    // are axis-aligned or diagonal (multiples of 45/90 degrees). Confirm
    // at least one segment direction is neither axis-aligned nor a
    // 45-degree diagonal -- geometry only the hex-based Gosper curve
    // produces among this codebase's plane-path fill patterns.
    bool found_hex_angle = false;
    for (const Polyline &pl : polylines) {
        for (size_t i = 1; i < pl.points.size() && !found_hex_angle; ++i) {
            Vec2d d = (pl.points[i] - pl.points[i - 1]).cast<double>();
            if (d.norm() < SCALED_EPSILON)
                continue;
            double angle_deg = std::abs(std::atan2(d.y(), d.x()) * 180.0 / M_PI);
            double remainder  = std::fmod(angle_deg, 45.0);
            if (remainder > 1.0 && remainder < 44.0)
                found_hex_angle = true;
        }
    }
    REQUIRE(found_hex_angle);
}
