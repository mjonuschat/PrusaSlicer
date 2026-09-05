#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "libslic3r/GCode/WipeTower.hpp"

using namespace Slic3r;

TEST_CASE("compute_load_move_x_advanced returns nullopt when loading_dist is zero", "[WipeTower]")
{
    auto result = compute_load_move_x_advanced(0.f, 50.f, 0.f, 20.f, 50.f);
    REQUIRE(! result.has_value());
}

TEST_CASE("compute_load_move_x_advanced returns nullopt when loading_speed is zero", "[WipeTower]")
{
    auto result = compute_load_move_x_advanced(0.f, 50.f, 10.f, 0.f, 50.f);
    REQUIRE(! result.has_value());
}

TEST_CASE("compute_load_move_x_advanced returns nullopt when both are zero", "[WipeTower]")
{
    auto result = compute_load_move_x_advanced(0.f, 50.f, 0.f, 0.f, 50.f);
    REQUIRE(! result.has_value());
}

TEST_CASE("compute_load_move_x_advanced computes a synchronized move when both are nonzero", "[WipeTower]")
{
    auto result = compute_load_move_x_advanced(0.f, 50.f, 10.f, 20.f, 150.f);
    REQUIRE(result.has_value());
    REQUIRE(result->x_speed == Catch::Approx(100.f));
    REQUIRE(result->end_x == Catch::Approx(50.f));
}

TEST_CASE("compute_load_move_x_advanced clamps x_speed and shortens the distance", "[WipeTower]")
{
    auto result = compute_load_move_x_advanced(0.f, 50.f, 10.f, 20.f, 50.f);
    REQUIRE(result.has_value());
    REQUIRE(result->x_speed == Catch::Approx(50.f));
    REQUIRE(result->end_x == Catch::Approx(25.f));
}

TEST_CASE("compute_load_move_x_advanced moves towards a farthest_x behind the current position", "[WipeTower]")
{
    auto result = compute_load_move_x_advanced(50.f, 0.f, 10.f, 20.f, 150.f);
    REQUIRE(result.has_value());
    REQUIRE(result->end_x == Catch::Approx(0.f));
}
