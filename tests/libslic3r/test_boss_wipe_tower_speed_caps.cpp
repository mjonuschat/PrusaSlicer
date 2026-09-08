#include <catch2/catch_test_macros.hpp>

#include <vector>

#include "libslic3r/GCode/WipeTower.hpp"

using namespace Slic3r;

TEST_CASE("apply_speed_caps composes independent caps via minimum", "[boss][toolchange]")
{
    const float target_speed = 100.f;

    SECTION("no caps leaves the target speed unchanged")
    {
        REQUIRE(WipeTower::apply_speed_caps(target_speed, {}) == target_speed);
    }

    SECTION("a single cap below the target speed wins")
    {
        REQUIRE(WipeTower::apply_speed_caps(target_speed, {60.f}) == 60.f);
    }

    SECTION("a single cap above the target speed has no effect")
    {
        REQUIRE(WipeTower::apply_speed_caps(target_speed, {150.f}) == target_speed);
    }

    SECTION("multiple caps fold to their minimum, regardless of order")
    {
        REQUIRE(WipeTower::apply_speed_caps(target_speed, {60.f, 40.f, 80.f}) == 40.f);
        REQUIRE(WipeTower::apply_speed_caps(target_speed, {80.f, 40.f, 60.f}) == 40.f);
        REQUIRE(WipeTower::apply_speed_caps(target_speed, {40.f, 80.f, 60.f}) == 40.f);
    }

    SECTION("a cap exactly equal to the target speed has no effect")
    {
        REQUIRE(WipeTower::apply_speed_caps(target_speed, {target_speed}) == target_speed);
    }
}
