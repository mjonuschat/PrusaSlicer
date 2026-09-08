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

TEST_CASE("Infill flip resolution alternates by layer parity even when travel chaining disagrees", "[boss][gcode]")
{
    using Slic3r::Boss::ReverseOddLayerPolicy;

    // With the option off, the caller's own travel-chaining decision passes through
    // unchanged, on both parities.
    REQUIRE(ReverseOddLayerPolicy::resolve_infill_flip(/*natural_flipped=*/true, /*is_odd_layer=*/false, /*infill_reverse=*/false) == true);
    REQUIRE(ReverseOddLayerPolicy::resolve_infill_flip(/*natural_flipped=*/false, /*is_odd_layer=*/true, /*infill_reverse=*/false) == false);

    // Adversarial case: chaining picks natural_flipped=true on the even layer and
    // natural_flipped=false on the odd layer -- the opposite of what a naive XOR of
    // reverse_infill onto natural_flipped would need to keep the layers alternating.
    // With the option on, both layers must ignore that and use layer parity alone,
    // so the resolved flips must differ between the two layers regardless.
    const bool even_flip = ReverseOddLayerPolicy::resolve_infill_flip(/*natural_flipped=*/true, /*is_odd_layer=*/false, /*infill_reverse=*/true);
    const bool odd_flip = ReverseOddLayerPolicy::resolve_infill_flip(/*natural_flipped=*/false, /*is_odd_layer=*/true, /*infill_reverse=*/true);
    REQUIRE(even_flip != odd_flip);

    // And the resolved values must come from parity, not from natural_flipped: flipping
    // what chaining decided for either layer must not change the result.
    REQUIRE(ReverseOddLayerPolicy::resolve_infill_flip(/*natural_flipped=*/false, /*is_odd_layer=*/false, /*infill_reverse=*/true) == even_flip);
    REQUIRE(ReverseOddLayerPolicy::resolve_infill_flip(/*natural_flipped=*/true, /*is_odd_layer=*/true, /*infill_reverse=*/true) == odd_flip);
}
