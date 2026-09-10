#include <algorithm>

#include <catch2/catch_test_macros.hpp>

#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefsFDM.hpp"

namespace {

const Slic3r::Domain::ConfigItemDef* find_def(const std::string& name)
{
    const Slic3r::Domain::ConfigDefinitions& defs = Slic3r::Domain::get_defs_fdm();
    const auto it = std::find_if(
        defs.defs().begin(), defs.defs().end(),
        [&name](const Slic3r::Domain::ConfigItemDef& def) { return def.name == name; }
    );
    return it != defs.defs().end() ? &*it : nullptr;
}

} // namespace

TEST_CASE("first_layer_flow_ratio is registered with the expected range and default", "[boss][config]")
{
    const Slic3r::Domain::ConfigItemDef* def = find_def("first_layer_flow_ratio");
    REQUIRE(def != nullptr);
    CHECK(def->min == 0.5);
    CHECK(def->max == 1.5);
}

TEST_CASE("top_layer_flow_ratio is registered with the expected range and default", "[boss][config]")
{
    const Slic3r::Domain::ConfigItemDef* def = find_def("top_layer_flow_ratio");
    REQUIRE(def != nullptr);
    CHECK(def->min == 0.5);
    CHECK(def->max == 1.5);
}
