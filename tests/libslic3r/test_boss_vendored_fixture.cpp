// Compiles only when BOSS_FEATURES_DIR points at
// src/boss/include/boss/test-fixtures. Proves a manifest's 'vendored' entry
// reaches real CMake: the generated add_library()/target_include_
// directories()/target_link_libraries() calls actually let libslic3r's own
// source compile against the vendored header and link.
#include <catch2/catch_test_macros.hpp>

#include "boss/test-fixtures/vendored-fixture/VendoredFixtureFeature.hpp"
#include "libslic3r/boss/test_fixtures/VendoredFixture.hpp"

TEST_CASE("A generator-discovered vendored fixture is compiled and linked into libslic3r", "[boss][fixture]")
{
    REQUIRE(Slic3r::Boss::Test::vendored_fixture_marker()
            == Slic3r::Boss::Test::VendoredFixtureFeature::id);
}
