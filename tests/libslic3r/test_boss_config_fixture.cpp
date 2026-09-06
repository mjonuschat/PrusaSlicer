// Compiles only when BOSS_FEATURES_DIR points at
// src/boss/include/boss/test-fixtures. Proves the real
// fdm_config_init_fn() call site registers a generator-discovered
// feature, not a synthetic instantiation of the registry template.
#include <algorithm>

#include <catch2/catch_test_macros.hpp>

#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefsFDM.hpp"

TEST_CASE("A generator-discovered config fixture reaches the real fdm_config_init_fn()", "[boss][config][fixture]")
{
    const Slic3r::Domain::ConfigDefinitions& defs = Slic3r::Domain::get_defs_fdm();
    const bool found = std::any_of(
        defs.defs().begin(), defs.defs().end(),
        [](const Slic3r::Domain::ConfigItemDef& def) { return def.name == "boss_test_fixture_flag"; }
    );
    REQUIRE(found);
}
