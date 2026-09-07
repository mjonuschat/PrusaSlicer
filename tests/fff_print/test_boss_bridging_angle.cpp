#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cmath>
#include <optional>

#include "boss/features/flowsnake/FlowsnakeFeature.hpp"
#include "boss/generated/BossFills.hpp"
#include "libslic3r/libslic3r.h"

using Catch::Approx;

namespace Slic3r {
double boss_flowsnake_bridging_angle_addition(std::optional<int> boss_fill_pattern_id);
} // namespace Slic3r

using namespace Slic3r;

TEST_CASE("Flowsnake adds a fixed 30 degree offset to the already-computed bridging angle", "[boss][infill]")
{
    REQUIRE(boss_flowsnake_bridging_angle_addition(std::optional<int>{Boss::FlowsnakeFeature::id}) == Approx((1.0 / 6.0) * PI));
}

TEST_CASE("No offset is added when the dominant pattern is not Flowsnake", "[boss][infill]")
{
    REQUIRE(boss_flowsnake_bridging_angle_addition(std::nullopt) == 0.0);
}
