// src/boss/include/boss/test-fixtures/fill-fixture/FillFixtureFeature.hpp
//
// Test-only fixture proving the fill-dispatch hook (BossFills, Fill.cpp's
// two dispatch points) actually threads a real, generator-discovered
// manifest through to a real compiled-in Fill subtype -- never a real
// BOSS feature, never discovered by a default build.
#pragma once

#include <memory>
#include <string_view>

namespace Slic3r {
class Fill;
}

namespace Slic3r::Boss::Test {

struct FillFixtureFeature {
    static constexpr int id = 900102;
    static constexpr std::string_view key = "boss-test-fill-fixture";
    static constexpr std::string_view label = "BOSS test fill fixture";
    static constexpr bool anchoring_eligible = true;

    static std::unique_ptr<Fill> create_fill();
    static bool use_bridge_flow();
};

} // namespace Slic3r::Boss::Test
