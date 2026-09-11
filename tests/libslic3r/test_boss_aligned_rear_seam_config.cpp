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

TEST_CASE("Aligned Rear is offered as a seam position", "[boss][seam][config]")
{
    const Domain::ConfigItemDef *def = find_option("seam_position");
    REQUIRE(def != nullptr);

    const Domain::EnumValueDefs &choices = def->init_fn().get<Domain::EnumWrapper>().def();

    const auto aligned_rear = std::find_if(
        choices.begin(),
        choices.end(),
        [](const Domain::EnumValueDef &choice) { return choice.str_serialized == "aligned_rear"; }
    );
    REQUIRE(aligned_rear != choices.end());
    CHECK(aligned_rear->enum_value == int(Domain::SeamPosition::spAlignedRear));

    CHECK(choices.size() == 5);
    CHECK(std::is_sorted(choices.begin(), choices.end()));
    for (const std::string &native : {"random", "nearest", "aligned", "rear"})
        CHECK(std::any_of(choices.begin(), choices.end(),
                          [&native](const Domain::EnumValueDef &choice)
                          { return choice.str_serialized == native; }));
}
