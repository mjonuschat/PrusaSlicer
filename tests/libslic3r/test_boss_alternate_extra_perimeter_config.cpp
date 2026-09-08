#include <algorithm>

#include <catch2/catch_test_macros.hpp>

#include "Slic3r/Domain/ConfigDefsFDM.hpp"

TEST_CASE("BOSS alternate-extra-perimeter config option is registered", "[boss][config]")
{
    const Slic3r::Domain::ConfigDefinitions& defs = Slic3r::Domain::get_defs_fdm();
    const bool found = std::any_of(
        defs.defs().begin(), defs.defs().end(),
        [](const Slic3r::Domain::ConfigItemDef& def) { return def.name == "alternate_extra_perimeter"; }
    );
    REQUIRE(found);
}
