// src/boss/include/boss/test-fixtures/capabilityless-fixture/CapabilitylessFixtureFeature.hpp
//
// Test-only fixture proving a manifest component that declares 'sources'
// but no 'capabilities' still gets its source file compiled into its
// target purely through the generator (no hand-edit to that target's own
// CMakeLists.txt). This feature joins no capability registry, so this
// trait is never referenced by any generated composition header -- it
// exists only so the manifest has a syntactically valid trait/header pair.
#pragma once

#include <string_view>

namespace Slic3r::Boss::Test {

struct CapabilitylessFixtureFeature {
    static constexpr int id = 900103;
    static constexpr std::string_view key = "boss-test-capabilityless-fixture";
    static constexpr std::string_view label = "BOSS test capability-less fixture";
};

} // namespace Slic3r::Boss::Test
