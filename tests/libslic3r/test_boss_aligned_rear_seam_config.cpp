#include <algorithm>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "Slic3r/Domain/ConfigDefsFDM.hpp"

using namespace Slic3r;

namespace {

const Domain::ConfigItemDef *find_option(const std::string &name)
{
    const Domain::ConfigDefinitions &defs = Domain::get_defs_fdm();
    auto it                               = std::find_if(
        defs.defs().begin(),
        defs.defs().end(),
        [&name](const Domain::ConfigItemDef &def) { return def.name == name; }
    );
    return it == defs.defs().end() ? nullptr : &*it;
}

} // namespace

TEST_CASE("seam_position_aligned_rear registers as an opt-in bool", "[boss][seam][config]")
{
    const Domain::ConfigItemDef *def = find_option("seam_position_aligned_rear");
    REQUIRE(def != nullptr);
    CHECK(def->category == Domain::ConfigItemDef::Category::Print_WallsPerimeters);
    CHECK(def->option_group == Domain::ConfigItemDef::OptionGroup::Print_WallsPerimeters_Seams);
    CHECK(def->init_fn().get<bool>() == false);
}
