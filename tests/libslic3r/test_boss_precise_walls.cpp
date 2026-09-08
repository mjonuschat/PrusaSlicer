#include <catch2/catch_test_macros.hpp>

#include "libslic3r/Flow.hpp"
#include "libslic3r/libslic3r.h"
#include "libslic3r/boss/perimeter/overlap/PreciseWalls.hpp"
#include "Slic3r/Domain/Percentage.hpp"

using Slic3r::Flow;
using Slic3r::Boss::PreciseWalls;
using Slic3r::Domain::FloatOrPercentage;
using Slic3r::Domain::Percentage;

namespace {

Flow test_flow()
{
    // width, height, nozzle_diameter (mm)
    return Flow(0.45f, 0.2f, 0.4f);
}

} // namespace

TEST_CASE("PreciseWalls::calculate_perimeter_spacing shrinks as overlap grows", "[boss][perimeter]")
{
    const Flow flow = test_flow();

    const coord_t low_overlap  = PreciseWalls::calculate_perimeter_spacing(flow, FloatOrPercentage(Percentage{10.}));
    const coord_t high_overlap = PreciseWalls::calculate_perimeter_spacing(flow, FloatOrPercentage(Percentage{50.}));

    REQUIRE(high_overlap < low_overlap);
}

TEST_CASE("PreciseWalls::calculate_external_spacing shrinks as overlap grows", "[boss][perimeter]")
{
    const Flow ext_flow = test_flow();
    const Flow int_flow = test_flow();

    const coord_t low_overlap  = PreciseWalls::calculate_external_spacing(
        ext_flow, int_flow, FloatOrPercentage(Percentage{10.}));
    const coord_t high_overlap = PreciseWalls::calculate_external_spacing(
        ext_flow, int_flow, FloatOrPercentage(Percentage{50.}));

    REQUIRE(high_overlap < low_overlap);
}

TEST_CASE("PreciseWalls::get_effective_perimeter_overlap no-ops below 3 perimeters", "[boss][perimeter]")
{
    const FloatOrPercentage user_overlap{Percentage{40.}};

    const FloatOrPercentage with_two = PreciseWalls::get_effective_perimeter_overlap(user_overlap, 2);
    REQUIRE(with_two.is_percentage());
    REQUIRE(with_two.percentage().value == PreciseWalls::get_standard_overlap_percent());

    const FloatOrPercentage with_three = PreciseWalls::get_effective_perimeter_overlap(user_overlap, 3);
    REQUIRE(with_three == user_overlap);
}

TEST_CASE("PreciseWalls::get_effective_perimeter_overlap clamps above 80%", "[boss][perimeter]")
{
    const FloatOrPercentage user_overlap{Percentage{95.}};

    const FloatOrPercentage effective = PreciseWalls::get_effective_perimeter_overlap(user_overlap, 3);
    REQUIRE(effective.is_percentage());
    REQUIRE(effective.percentage().value == 80.);
}

TEST_CASE("PreciseWalls::get_effective_external_overlap no-ops below 2 perimeters", "[boss][perimeter]")
{
    const FloatOrPercentage user_overlap{Percentage{40.}};

    const FloatOrPercentage with_one = PreciseWalls::get_effective_external_overlap(user_overlap, 1);
    REQUIRE(with_one.is_percentage());
    REQUIRE(with_one.percentage().value == PreciseWalls::get_standard_overlap_percent());

    const FloatOrPercentage with_two = PreciseWalls::get_effective_external_overlap(user_overlap, 2);
    REQUIRE(with_two == user_overlap);
}
