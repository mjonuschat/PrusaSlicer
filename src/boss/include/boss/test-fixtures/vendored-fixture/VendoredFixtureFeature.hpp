// src/boss/include/boss/test-fixtures/vendored-fixture/VendoredFixtureFeature.hpp
//
// Test-only fixture proving a manifest's 'vendored' entry actually reaches
// real CMake: emit_vendored_cmake() must declare the vendored library,
// point its include directory at this manifest's own vendor/ subdirectory,
// and link it into the component that consumes it -- never a real BOSS
// feature, never discovered by a default build.
#pragma once

#include <string_view>

namespace Slic3r::Boss::Test {

struct VendoredFixtureFeature {
    static constexpr int id = 900104;
    static constexpr std::string_view key = "boss-test-vendored-fixture";
    static constexpr std::string_view label = "BOSS test vendored fixture";
};

} // namespace Slic3r::Boss::Test
