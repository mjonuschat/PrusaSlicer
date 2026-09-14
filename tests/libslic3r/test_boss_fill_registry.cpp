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
    // An empty composition must leave fill_pattern exactly as upstream has it,
    // so register_config() adds nothing at all rather than a no-choice option.
    const bool found = std::any_of(
        defs.defs().begin(), defs.defs().end(),
        [](const Slic3r::Domain::ConfigItemDef &def) { return def.name == "fill_pattern"; }
    );
    REQUIRE_FALSE(found);
}
