// Compiles only when BOSS_FEATURES_DIR points at
// src/boss/include/boss/test-fixtures. Proves the generated field exists and
// ExtrudeConfig embeds the overrides struct.
#include <type_traits>

#include <catch2/catch_test_macros.hpp>

#include "boss/generated/BossExtrudeConfigOverrides.hpp"
#include "libslic3r/ExtrudeConfig.hpp"

static_assert(
    std::is_same_v<
        decltype(Slic3r::Boss::BossExtrudeConfigOverrides::boss_test_fixture_flag), bool>,
    "the fixture store field must be generated as a bool"
);
static_assert(
    std::is_same_v<
        decltype(Slic3r::Biz::Slicing::ExtrudeConfig::boss),
        Slic3r::Boss::BossExtrudeConfigOverrides>,
    "ExtrudeConfig must embed the generated overrides struct"
);

TEST_CASE("ExtrudeConfig embeds the generated overrides", "[boss][storage][fixture]")
{
    SUCCEED();
}
