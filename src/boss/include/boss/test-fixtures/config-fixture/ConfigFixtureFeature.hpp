// src/boss/include/boss/test-fixtures/config-fixture/ConfigFixtureFeature.hpp
//
// Test-only fixture proving the config-registration hook (BossFdmFeatures,
// fdm_config_init_fn()) actually threads a real, generator-discovered
// manifest through to a real compiled-in registration -- never a real
// BOSS feature, never discovered by a default build. Only reachable by a
// build that overrides BOSS_FEATURES_DIR to
// src/boss/include/boss/test-fixtures.
#pragma once

#include <string_view>

namespace Slic3r::Domain {
class ConfigDefinitions;
}

namespace Slic3r::Boss::Test {

struct ConfigFixtureFeature {
    static constexpr int id = 900101;
    static constexpr std::string_view key = "boss-test-config-fixture";
    static constexpr std::string_view label = "BOSS test config fixture";

    static void register_config(Domain::ConfigDefinitions& defs);
};

} // namespace Slic3r::Boss::Test
