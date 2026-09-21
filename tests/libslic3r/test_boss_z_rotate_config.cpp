#include <algorithm>

#include <catch2/catch_test_macros.hpp>

#include "Slic3r/Domain/ConfigDefsFDM.hpp"
#include "Slic3r/Domain/ConfigDef.hpp"

TEST_CASE("BOSS z-rotate config option is registered", "[boss][config]")
{
    const Slic3r::Domain::ConfigDefinitions& defs = Slic3r::Domain::get_defs_fdm();
    const bool found = std::any_of(
        defs.defs().begin(), defs.defs().end(),
        [](const Slic3r::Domain::ConfigItemDef& def) { return def.name == "init_z_rotate"; }
    );
    REQUIRE(found);
}
