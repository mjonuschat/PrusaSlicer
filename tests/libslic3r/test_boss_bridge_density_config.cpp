#include <algorithm>

#include <catch2/catch_test_macros.hpp>

#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefsFDM.hpp"

TEST_CASE("bridge_density is registered with the expected range and default", "[boss][config]")
{
    const Slic3r::Domain::ConfigDefinitions& defs = Slic3r::Domain::get_defs_fdm();
    const auto it = std::find_if(
        defs.defs().begin(), defs.defs().end(),
        [](const Slic3r::Domain::ConfigItemDef& def) { return def.name == "bridge_density"; }
    );
    REQUIRE(it != defs.defs().end());
    CHECK(it->min == 10.);
    CHECK(it->max == 120.);
}
