#include <algorithm>

#include <catch2/catch_test_macros.hpp>

#include "Slic3r/Domain/ConfigDefsFDM.hpp"

namespace {

bool has_option(const std::string& name)
{
    const Slic3r::Domain::ConfigDefinitions& defs = Slic3r::Domain::get_defs_fdm();
    return std::any_of(
        defs.defs().begin(), defs.defs().end(),
        [&name](const Slic3r::Domain::ConfigItemDef& def) { return def.name == name; }
    );
}

} // namespace

TEST_CASE("BOSS reverse-odd-layer-extrusion config options are registered", "[boss][config]")
{
    REQUIRE(has_option("overhangs_reverse"));
    REQUIRE(has_option("internal_perimeters_reverse"));
    REQUIRE(has_option("infill_reverse"));
}
