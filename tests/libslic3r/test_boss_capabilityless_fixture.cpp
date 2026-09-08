// Compiles only when BOSS_FEATURES_DIR points at
// src/boss/include/boss/test-fixtures. Proves a manifest component that
// declares 'sources' but no 'capabilities' gets its source file compiled
// into libslic3r purely through the generator (boss_target_sources()),
// with no hand-edit to libslic3r's own CMakeLists.txt.
#include <catch2/catch_test_macros.hpp>

#include "libslic3r/boss/test_fixtures/CapabilitylessFixture.hpp"
#include "boss/test-fixtures/capabilityless-fixture/CapabilitylessFixtureFeature.hpp"

TEST_CASE("A generator-discovered capability-less fixture is compiled and linked into libslic3r", "[boss][fixture]")
{
    REQUIRE(Slic3r::Boss::Test::capabilityless_fixture_marker()
            == Slic3r::Boss::Test::CapabilitylessFixtureFeature::id);
}
