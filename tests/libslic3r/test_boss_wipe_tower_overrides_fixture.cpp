// Compiles only when BOSS_FEATURES_DIR points at
// src/boss/include/boss/test-fixtures. Proves the generated wipe_tower field
// exists and that WipeTower embeds the overrides struct.
#include <type_traits>

#include <catch2/catch_test_macros.hpp>

#include "boss/generated/BossWipeTowerOverrides.hpp"
#include "libslic3r/GCode/WipeTower.hpp"

static_assert(
    std::is_same_v<decltype(Slic3r::Boss::BossWipeTowerOverrides::boss_test_fixture_purge), bool>,
    "the wipe_tower fixture store field must be generated as a bool"
);
static_assert(
    std::is_same_v<decltype(Slic3r::WipeTower::boss), Slic3r::Boss::BossWipeTowerOverrides>,
    "WipeTower must embed the generated overrides struct"
);

TEST_CASE("WipeTower embeds the generated overrides", "[boss][storage][fixture]")
{
    SUCCEED();
}
