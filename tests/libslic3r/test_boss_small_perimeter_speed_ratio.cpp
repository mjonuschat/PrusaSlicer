#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "libslic3r/boss/perimeter/small_threshold/SmallPerimeterSpeedRatio.hpp"

TEST_CASE("Small perimeter speed ratio is 1.0 at or below min length", "[boss][perimeters]")
{
    REQUIRE(Slic3r::Boss::SmallPerimeterSpeedRatio::speed_ratio(30.0, 40.0, 125.0) == 1.0);
}

TEST_CASE("Small perimeter speed ratio is 0.0 at or above max length", "[boss][perimeters]")
{
    REQUIRE(Slic3r::Boss::SmallPerimeterSpeedRatio::speed_ratio(200.0, 40.0, 125.0) == 0.0);
}

TEST_CASE("Small perimeter speed ratio interpolates linearly between min and max", "[boss][perimeters]")
{
    const double ratio = Slic3r::Boss::SmallPerimeterSpeedRatio::speed_ratio(82.5, 40.0, 125.0);
    REQUIRE(ratio == Catch::Approx(0.5));
}
