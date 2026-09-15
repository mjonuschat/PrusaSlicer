#include <algorithm>

#include <catch2/catch_test_macros.hpp>

#include "Slic3r/Domain/ConfigDefsFDM.hpp"
#include "Slic3r/Domain/ConfigDef.hpp"

TEST_CASE("BOSS wipe-tower ramming/cooling disable options are registered", "[boss][config]")
{
    const Slic3r::Domain::ConfigDefinitions& defs = Slic3r::Domain::get_defs_fdm();
    for (const std::string& key : {"wipe_tower_disable_filament_ramming", "wipe_tower_disable_cooling_moves"}) {
        const bool found = std::any_of(
            defs.defs().begin(), defs.defs().end(),
            [&](const Slic3r::Domain::ConfigItemDef& def) { return def.name == key; }
        );
        INFO("missing key: " << key);
        REQUIRE(found);
    }
}
