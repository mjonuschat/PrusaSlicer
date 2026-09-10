// src/boss/include/boss/test-fixtures/solid-fill-policy-fixture/SolidFillPolicyFixtureFeature.hpp
//
// Test-only fixture proving the solid_fill_policy registry's two Fill.cpp
// call sites actually reach a real, generator-discovered feature, and that
// force_ensuring wins over preferred_pattern -- never a real BOSS feature,
// never discovered by a default build. Only reachable by a build that
// overrides BOSS_FEATURES_DIR to src/boss/include/boss/test-fixtures.
#pragma once

#include <optional>
#include <string_view>

#include "Slic3r/Domain/ConfigDefsFDM.hpp"

namespace Slic3r::Domain {
class ConfigDefinitions;
}

namespace Slic3r::Boss {
struct SolidFillPolicyContext;
}

namespace Slic3r::Boss::Test {

struct SolidFillPolicyFixtureFeature {
    static constexpr int id = 900501;
    static constexpr std::string_view key = "boss-test-solid-fill-policy-fixture";
    static constexpr std::string_view label = "BOSS test solid-fill-policy fixture";

    static void register_config(Domain::ConfigDefinitions &defs);
    static bool force_ensuring(const Slic3r::Boss::SolidFillPolicyContext &ctx);
    static std::optional<Domain::InfillPattern> preferred_pattern(const Slic3r::Boss::SolidFillPolicyContext &ctx);
};

} // namespace Slic3r::Boss::Test
