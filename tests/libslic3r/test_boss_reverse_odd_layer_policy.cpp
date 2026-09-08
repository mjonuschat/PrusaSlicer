#include <catch2/catch_test_macros.hpp>

#include "libslic3r/boss/gcode/reverse_odd_layer/ReverseOddLayerPolicy.hpp"

TEST_CASE("Internal perimeters reverse on odd layers when enabled", "[boss][gcode]")
{
    using Slic3r::Boss::ReverseOddLayerPolicy;
    REQUIRE(ReverseOddLayerPolicy::reverse_perimeter(
        /*is_odd_layer=*/true, /*is_internal=*/true, /*is_overhang=*/false,
        /*internal_perimeters_reverse=*/true, /*overhangs_reverse=*/false) == true);
    REQUIRE(ReverseOddLayerPolicy::reverse_perimeter(
        /*is_odd_layer=*/false, /*is_internal=*/true, /*is_overhang=*/false,
        /*internal_perimeters_reverse=*/true, /*overhangs_reverse=*/false) == false);
}

TEST_CASE("Overhang perimeters reverse on odd layers when enabled", "[boss][gcode]")
{
    using Slic3r::Boss::ReverseOddLayerPolicy;
    REQUIRE(ReverseOddLayerPolicy::reverse_perimeter(
        /*is_odd_layer=*/true, /*is_internal=*/false, /*is_overhang=*/true,
        /*internal_perimeters_reverse=*/false, /*overhangs_reverse=*/true) == true);
}

TEST_CASE("Infill reverses on odd layers when enabled", "[boss][gcode]")
{
    REQUIRE(Slic3r::Boss::ReverseOddLayerPolicy::reverse_infill(true, true) == true);
    REQUIRE(Slic3r::Boss::ReverseOddLayerPolicy::reverse_infill(false, true) == false);
    REQUIRE(Slic3r::Boss::ReverseOddLayerPolicy::reverse_infill(true, false) == false);
}
