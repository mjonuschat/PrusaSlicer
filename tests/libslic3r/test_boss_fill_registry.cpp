#include <catch2/catch_test_macros.hpp>

#include "Slic3r/Domain/ConfigDef.hpp"
#include "boss/foundation/BossFillRegistry.hpp"

TEST_CASE("BossFillRegistry<> (explicitly empty) registers no config option", "[boss][config]")
{
    Slic3r::Domain::ConfigDefinitions defs(
        {},
        [](Slic3r::Domain::ConfigDefinitions &defs) {
            Slic3r::Boss::BossFillRegistry<>::register_config(defs);
        }
    );
    const bool found = std::any_of(
        defs.defs().begin(), defs.defs().end(),
        [](const Slic3r::Domain::ConfigItemDef &def) { return def.name == "boss_fill_pattern"; }
    );
    REQUIRE_FALSE(found);
}
