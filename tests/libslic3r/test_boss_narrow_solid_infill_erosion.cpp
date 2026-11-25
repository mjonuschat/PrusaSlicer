#include <catch2/catch_test_macros.hpp>

#include "libslic3r/boss/surface/narrow_solid/NarrowSolidInfillErosion.hpp"
#include "libslic3r/Point.hpp"

using namespace Slic3r;

namespace {

ExPolygon rectangle(const Point& origin, const coord_t width, const coord_t height)
{
    return {
        origin,
        origin + Point{width, 0},
        origin + Point{width, height},
        origin + Point{0, height},
    };
}

} // namespace

TEST_CASE("A wide solid region is not classified as narrow", "[boss][surface]")
{
    const ExPolygons wide{rectangle({0, 0}, scaled(10.0), scaled(10.0))};
    REQUIRE(Slic3r::Boss::NarrowSolidInfillErosion::classify(wide, scaled(0.4), 1.0) == false);
}

TEST_CASE("A thin sliver region is classified as narrow", "[boss][surface]")
{
    const ExPolygons sliver{rectangle({0, 0}, scaled(10.0), scaled(0.3))};
    REQUIRE(Slic3r::Boss::NarrowSolidInfillErosion::classify(sliver, scaled(0.4), 1.0) == true);
}

TEST_CASE("A thin sliver with a small total area is narrow and small", "[boss][surface]")
{
    const ExPolygons sliver{rectangle({0, 0}, scaled(1.0), scaled(0.3))};
    REQUIRE(Slic3r::Boss::NarrowSolidInfillErosion::is_narrow_and_small(sliver, scaled(0.4), 1.0) == true);
}

TEST_CASE("A thin band with a large total area erodes away but is not classified as small", "[boss][surface]")
{
    const ExPolygons wide_band{rectangle({0, 0}, scaled(200.0), scaled(0.5))};
    REQUIRE(Slic3r::Boss::NarrowSolidInfillErosion::classify(wide_band, scaled(0.4), 1.5) == true);
    REQUIRE(Slic3r::Boss::NarrowSolidInfillErosion::is_narrow_and_small(wide_band, scaled(0.4), 1.5) == false);
}
