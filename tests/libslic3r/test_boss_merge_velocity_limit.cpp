#include <catch2/catch_test_macros.hpp>

#include "boss/features/merge-velocity-limit/MergeVelocityLimitFeature.hpp"

using namespace Slic3r::Boss;

TEST_CASE(
    "MergeVelocityLimitFeature merges consecutive SET_VELOCITY_LIMIT lines", "[boss][klipper]"
)
{
    std::string input =
        "G1 X10 Y10 E1\n"
        "SET_VELOCITY_LIMIT ACCEL=1000 MINIMUM_CRUISE_RATIO=0.5 ; adjust acceleration "
        "(Perimeter)\n"
        "SET_VELOCITY_LIMIT SQUARE_CORNER_VELOCITY=5 ; adjust jerk (Perimeter)\n"
        "G1 X20 Y20 E2\n";
    std::string expected =
        "G1 X10 Y10 E1\n"
        "SET_VELOCITY_LIMIT ACCEL=1000 MINIMUM_CRUISE_RATIO=0.5 SQUARE_CORNER_VELOCITY=5 ; "
        "adjust acceleration (Perimeter)\n"
        "G1 X20 Y20 E2\n";
    CHECK(MergeVelocityLimitFeature::filter_layer(input) == expected);
}

TEST_CASE(
    "MergeVelocityLimitFeature leaves a lone SET_VELOCITY_LIMIT line unchanged", "[boss][klipper]"
)
{
    std::string input = "SET_VELOCITY_LIMIT ACCEL=1000\nG1 X10 Y10 E1\n";
    CHECK(MergeVelocityLimitFeature::filter_layer(input) == input);
}

TEST_CASE("MergeVelocityLimitFeature leaves non-Klipper G-code untouched", "[boss][klipper]")
{
    std::string input = "M204 S1000\nM205 X5 Y5\nG1 X10 Y10 E1\n";
    CHECK(MergeVelocityLimitFeature::filter_layer(input) == input);
}

TEST_CASE(
    "MergeVelocityLimitFeature merges an ACCEL-only line with a following SCV-only line even "
    "without comments",
    "[boss][klipper]"
)
{
    std::string input =
        "SET_VELOCITY_LIMIT ACCEL=500\nSET_VELOCITY_LIMIT SQUARE_CORNER_VELOCITY=3\n";
    std::string expected = "SET_VELOCITY_LIMIT ACCEL=500 SQUARE_CORNER_VELOCITY=3\n";
    CHECK(MergeVelocityLimitFeature::filter_layer(input) == expected);
}

TEST_CASE(
    "MergeVelocityLimitFeature keeps the SCV line's comment when the ACCEL line has none",
    "[boss][klipper]"
)
{
    std::string input =
        "SET_VELOCITY_LIMIT ACCEL=1000\n"
        "SET_VELOCITY_LIMIT SQUARE_CORNER_VELOCITY=5 ; bar\n";
    std::string expected = "SET_VELOCITY_LIMIT ACCEL=1000 SQUARE_CORNER_VELOCITY=5 ; bar\n";
    CHECK(MergeVelocityLimitFeature::filter_layer(input) == expected);
}

TEST_CASE(
    "MergeVelocityLimitFeature returns the input unchanged when it has no SET_VELOCITY_LIMIT line",
    "[boss][klipper]"
)
{
    std::string input = "M204 S1000\nM205 X5 Y5\nG1 X10 Y10 E1"; // deliberately no trailing newline
    CHECK(MergeVelocityLimitFeature::filter_layer(input) == input);
}
