// fuzzy_polyline()/fuzzy_extrusion_line() now round the along-edge step and
// the perpendicular displacement separately (so the along-edge point can be
// sampled as a noise coordinate), instead of rounding their combined sum
// once -- for every noise type, Classic included. Proves that split is
// bounded to at most 1 scaled unit (1nm) per axis across a broad sample of
// edges/ratios/displacements, i.e. a no-op for any print-relevant purpose.
#include <cmath>

#include <catch2/catch_test_macros.hpp>

#include "Slic3r/Math.hpp"
#include "libslic3r/Point.hpp"

using Slic3r::Point;
using Slic3r::Vec2d;

namespace {

// The original (pre-feature) formula: a single combined double-vector sum, rounded once.
Point old_formula(const Point &p0, const Vec2d &p0p1, double ratio, const Vec2d &perp_unit, double r)
{
    return p0 + (p0p1 * ratio + perp_unit * r).cast<coord_t>();
}

// The current formula: the along-edge step is rounded into `pa` first (so it can be
// sampled as a noise coordinate), then the perpendicular displacement is rounded
// and added separately.
Point new_formula(const Point &p0, const Vec2d &p0p1, double ratio, const Vec2d &perp_unit, double r)
{
    const Point pa = p0 + (p0p1 * ratio).cast<coord_t>();
    return pa + (perp_unit * r).cast<coord_t>();
}

} // namespace

TEST_CASE("Classic mode's two-step point rounding differs from the original single-step rounding by at most 1 unit per axis", "[boss][surface]")
{
    const Point p0{Slic3r::scaled<coord_t>(0.0), Slic3r::scaled<coord_t>(0.0)};

    for (double angle_deg : {0.0, 15.0, 33.7, 45.0, 90.0, 123.4, 180.0, 271.1, 350.0}) {
        const double angle = angle_deg * M_PI / 180.0;
        const Vec2d  p0p1  = Slic3r::scaled<coord_t>(Vec2d{std::cos(angle), std::sin(angle)} * 5.0).cast<double>();
        const Vec2d  perp_unit = Slic3r::perp(p0p1).normalized();

        for (double ratio : {0.0, 0.1, 0.33, 0.5, 0.67, 0.9, 1.0}) {
            for (double thickness_mm : {0.05, 0.3, 1.0, 5.0}) {
                for (double r_fraction : {-1.0, -0.5, 0.0, 0.37, 1.0}) {
                    const double r = r_fraction * Slic3r::scaled<coord_t>(thickness_mm);

                    const Point old_point = old_formula(p0, p0p1, ratio, perp_unit, r);
                    const Point new_point = new_formula(p0, p0p1, ratio, perp_unit, r);

                    CHECK(std::abs(old_point.x() - new_point.x()) <= 1);
                    CHECK(std::abs(old_point.y() - new_point.y()) <= 1);
                }
            }
        }
    }
}
