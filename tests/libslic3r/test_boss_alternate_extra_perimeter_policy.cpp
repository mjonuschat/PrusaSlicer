#include <catch2/catch_test_macros.hpp>

#include "boss/features/alternate-extra-perimeter/AlternateExtraPerimeterFeature.hpp"
#include "libslic3r/boss/perimeter/PerimeterPolicyContext.hpp"

TEST_CASE("Alternate extra perimeter adds one loop on odd layers with density and no spiral vase",
          "[boss][perimeter]")
{
    Slic3r::Boss::PerimeterPolicyContext ctx;
    ctx.layer_id = 1;
    ctx.fill_density = 20.0;
    ctx.spiral_vase = false;
    REQUIRE(Slic3r::Boss::AlternateExtraPerimeterFeature::adjust_loop_count(2, ctx) == 3);
}

TEST_CASE("Alternate extra perimeter does nothing on even layers", "[boss][perimeter]")
{
    Slic3r::Boss::PerimeterPolicyContext ctx;
    ctx.layer_id = 2;
    ctx.fill_density = 20.0;
    REQUIRE(Slic3r::Boss::AlternateExtraPerimeterFeature::adjust_loop_count(2, ctx) == 2);
}

TEST_CASE("Alternate extra perimeter does nothing in spiral vase mode", "[boss][perimeter]")
{
    Slic3r::Boss::PerimeterPolicyContext ctx;
    ctx.layer_id = 1;
    ctx.fill_density = 20.0;
    ctx.spiral_vase = true;
    REQUIRE(Slic3r::Boss::AlternateExtraPerimeterFeature::adjust_loop_count(2, ctx) == 2);
}

TEST_CASE("Alternate extra perimeter does nothing at zero fill density", "[boss][perimeter]")
{
    Slic3r::Boss::PerimeterPolicyContext ctx;
    ctx.layer_id = 1;
    ctx.fill_density = 0.0;
    REQUIRE(Slic3r::Boss::AlternateExtraPerimeterFeature::adjust_loop_count(2, ctx) == 2);
}
