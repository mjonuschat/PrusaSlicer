#include <algorithm>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "boss/features/nip-tuck-seam/NipTuckSeamFeature.hpp"

#include "Slic3r/Domain/ConfigDefsFDM.hpp"
#include "Slic3r/Domain/ConfigValue.hpp"

using namespace Slic3r;
using namespace Slic3r::Boss;

namespace {

const Domain::ConfigItemDef* find_option(const std::string& name)
{
    const Domain::ConfigDefinitions& defs = Domain::get_defs_fdm();
    auto it                               = std::find_if(
        defs.defs().begin(),
        defs.defs().end(),
        [&name](const Domain::ConfigItemDef& def) { return def.name == name; }
    );
    return it == defs.defs().end() ? nullptr : &*it;
}

} // namespace

TEST_CASE("seam_type registers as a BOSS-owned enum defaulting to regular", "[boss][seam][config]")
{
    const Domain::ConfigItemDef* def = find_option("seam_type");
    REQUIRE(def != nullptr);
    CHECK(def->category == Domain::ConfigItemDef::Category::Print_WallsPerimeters);
    CHECK(def->option_group == Domain::ConfigItemDef::OptionGroup::Print_WallsPerimeters_Seams);
    CHECK(def->init_fn().get<NipTuckSeamType>() == NipTuckSeamType::Regular);
}

TEST_CASE("seam_type serializes with the keys preset round-tripping depends on", "[boss][seam][config]")
{
    const Domain::ConfigItemDef* def = find_option("seam_type");
    REQUIRE(def != nullptr);

    const Domain::EnumValueDefs& values = def->init_fn().get<Domain::EnumWrapper>().def();
    std::vector<std::string> keys;
    for (const Domain::EnumValueDef& value : values)
        keys.push_back(value.str_serialized);

    for (const std::string& expected : {"regular", "niptuck", "nip", "tuck", "alternating"})
        CHECK(std::find(keys.begin(), keys.end(), expected) != keys.end());
}

TEST_CASE("seam_notch_width registers with the documented 1-3x bounds and default 2.0", "[boss][seam][config]")
{
    const Domain::ConfigItemDef* def = find_option("seam_notch_width");
    REQUIRE(def != nullptr);
    REQUIRE(def->min.has_value());
    CHECK(*def->min == 1.0);
    REQUIRE(def->max.has_value());
    CHECK(*def->max == 3.0);
    CHECK(def->init_fn().get<double>() == 2.0);
}

TEST_CASE("seam_notch_angle registers with the documented 0-90 bounds and default 44.0", "[boss][seam][config]")
{
    const Domain::ConfigItemDef* def = find_option("seam_notch_angle");
    REQUIRE(def != nullptr);
    REQUIRE(def->min.has_value());
    CHECK(*def->min == 0.0);
    REQUIRE(def->max.has_value());
    CHECK(*def->max == 90.0);
    CHECK(def->init_fn().get<double>() == 44.0);
}
