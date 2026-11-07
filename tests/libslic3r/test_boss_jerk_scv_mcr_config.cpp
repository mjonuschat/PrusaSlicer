#include <algorithm>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "Slic3r/Domain/ConfigDefsFDM.hpp"

using namespace Slic3r;

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

TEST_CASE("JerkScvMcrFeature registers 24 options with correct bounds", "[boss][config]")
{
    // first_layer_over_raft's keys put "_over_raft" after the metric name,
    // matching the existing first_layer_acceleration_over_raft option, so it is
    // listed with its full key pair rather than derived from a role name.
    struct RolePair
    {
        const char* mcr_key;
        const char* jerk_key;
    };

    const RolePair roles[] = {
        {"external_perimeter_minimum_cruise_ratio", "external_perimeter_jerk"},
        {"perimeter_minimum_cruise_ratio", "perimeter_jerk"},
        {"top_solid_infill_minimum_cruise_ratio", "top_solid_infill_jerk"},
        {"solid_infill_minimum_cruise_ratio", "solid_infill_jerk"},
        {"infill_minimum_cruise_ratio", "infill_jerk"},
        {"bridge_minimum_cruise_ratio", "bridge_jerk"},
        {"first_layer_minimum_cruise_ratio", "first_layer_jerk"},
        {"first_layer_minimum_cruise_ratio_over_raft", "first_layer_jerk_over_raft"},
        {"wipe_tower_minimum_cruise_ratio", "wipe_tower_jerk"},
        {"travel_minimum_cruise_ratio", "travel_jerk"},
        {"travel_short_distance_minimum_cruise_ratio", "travel_short_distance_jerk"},
        {"default_minimum_cruise_ratio", "default_jerk"},
    };

    for (const RolePair& role : roles) {
        INFO("role mcr key: " << role.mcr_key);
        const std::string mcr_key  = role.mcr_key;
        const std::string jerk_key = role.jerk_key;

        const Domain::ConfigItemDef* mcr = find_option(mcr_key);
        REQUIRE(mcr != nullptr);
        REQUIRE(mcr->min.has_value());
        CHECK(*mcr->min == 0.0);
        REQUIRE(mcr->max.has_value());
        CHECK(*mcr->max == 0.999999);

        const Domain::ConfigItemDef* jerk = find_option(jerk_key);
        REQUIRE(jerk != nullptr);
        REQUIRE(jerk->min.has_value());
        CHECK(*jerk->min == 0.0);
    }
}

TEST_CASE("JerkScvMcrFeature bridge tooltip names bridges, not another role", "[boss][config]")
{
    const Domain::ConfigItemDef* bridge_mcr = find_option("bridge_minimum_cruise_ratio");
    REQUIRE(bridge_mcr != nullptr);
    CHECK(bridge_mcr->tooltip.find("bridges") != std::string::npos);

    const Domain::ConfigItemDef* perimeter_mcr = find_option("perimeter_minimum_cruise_ratio");
    REQUIRE(perimeter_mcr != nullptr);
    CHECK(perimeter_mcr->tooltip.find("bridges") == std::string::npos);
    CHECK(perimeter_mcr->tooltip.find("perimeters") != std::string::npos);
}
