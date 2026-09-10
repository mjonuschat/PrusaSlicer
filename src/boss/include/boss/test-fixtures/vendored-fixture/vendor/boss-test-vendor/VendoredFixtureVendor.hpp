// src/boss/include/boss/test-fixtures/vendored-fixture/vendor/boss-test-vendor/VendoredFixtureVendor.hpp
//
// Stand-in for a real vendored third-party header.
// Trivial on purpose: this fixture exists only to prove the generated
// add_library()/target_include_directories()/target_link_libraries() wiring
// compiles and links, not to vendor anything real.
#pragma once

namespace Slic3r::Boss::Test {

inline constexpr int vendored_fixture_vendor_marker = 900104;

} // namespace Slic3r::Boss::Test
