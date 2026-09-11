///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "boss/features/jerk-scv-mcr/JerkScvMcrFeature.hpp"
#include "boss/foundation/BossL.hpp"
#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefUtils.hpp"

namespace Slic3r::Boss {

namespace {

struct RoleSpec
{
    const char* mcr_key;
    const char* jerk_key;
    const char* tooltip_phrase;
    const char* label_prefix;
    // Matches the role's native acceleration order, spaced by 10 in
    // ConfigDefsFDM.cpp, so a role's three options stay together in the group.
    int accel_order;
    Domain::ConfigItemDef::OptionGroup option_group;
};

// Registration order is unrelated to the role-priority cascade (that order lives
// in JerkScvMcrPolicy.cpp) -- only the key/label/option_group per role matters here.
//
// first_layer_over_raft's keys put "_over_raft" after the metric name
// (first_layer_minimum_cruise_ratio_over_raft), matching the existing
// first_layer_acceleration_over_raft option this feature sits next to --
// not the "first_layer_over_raft_minimum_cruise_ratio" shape a naive
// role-prefix-plus-suffix scheme would produce for every other role.
constexpr RoleSpec kRoles[] = {
    {"external_perimeter_minimum_cruise_ratio",
     "external_perimeter_jerk",
     "external perimeters",
     "External perimeters",
     10,
     Domain::ConfigItemDef::OptionGroup::Print_MotionDynamics_MainStructureAcceleration},
    {"perimeter_minimum_cruise_ratio",
     "perimeter_jerk",
     "perimeters",
     "Perimeters",
     20,
     Domain::ConfigItemDef::OptionGroup::Print_MotionDynamics_MainStructureAcceleration},
    {"top_solid_infill_minimum_cruise_ratio",
     "top_solid_infill_jerk",
     "top solid infill",
     "Top solid infill",
     50,
     Domain::ConfigItemDef::OptionGroup::Print_MotionDynamics_MainStructureAcceleration},
    {"solid_infill_minimum_cruise_ratio",
     "solid_infill_jerk",
     "solid infill",
     "Solid infill",
     40,
     Domain::ConfigItemDef::OptionGroup::Print_MotionDynamics_MainStructureAcceleration},
    {"infill_minimum_cruise_ratio",
     "infill_jerk",
     "infill",
     "Infill",
     30,
     Domain::ConfigItemDef::OptionGroup::Print_MotionDynamics_MainStructureAcceleration},
    {"bridge_minimum_cruise_ratio",
     "bridge_jerk",
     "bridges",
     "Bridges",
     0,
     Domain::ConfigItemDef::OptionGroup::Print_MotionDynamics_BridgesAcceleration},
    {"first_layer_minimum_cruise_ratio",
     "first_layer_jerk",
     "the first layer",
     "First layer",
     0,
     Domain::ConfigItemDef::OptionGroup::Print_MotionDynamics_FirstLayerAcceleration},
    {"first_layer_minimum_cruise_ratio_over_raft",
     "first_layer_jerk_over_raft",
     "the first object layer over a raft interface",
     "First object layer over raft interface",
     10,
     Domain::ConfigItemDef::OptionGroup::Print_MotionDynamics_FirstLayerAcceleration},
    {"wipe_tower_minimum_cruise_ratio",
     "wipe_tower_jerk",
     "the wipe tower",
     "Wipe tower",
     0,
     Domain::ConfigItemDef::OptionGroup::Print_MotionDynamics_WipeTowerAcceleration},
    {"travel_minimum_cruise_ratio",
     "travel_jerk",
     "travel moves",
     "Travel",
     0,
     Domain::ConfigItemDef::OptionGroup::Print_MotionDynamics_TravelsAcceleration},
    {"travel_short_distance_minimum_cruise_ratio",
     "travel_short_distance_jerk",
     "short travel moves",
     "Travel short distance",
     10,
     Domain::ConfigItemDef::OptionGroup::Print_MotionDynamics_TravelsAcceleration},
    {"default_minimum_cruise_ratio",
     "default_jerk",
     "everything else",
     "Default",
     0,
     Domain::ConfigItemDef::OptionGroup::Print_MotionDynamics_MainStructureAcceleration},
};

void register_role(Domain::ConfigDefinitions& defs, const RoleSpec& role)
{
    using namespace Slic3r::Domain;

    ConfigItemDef* mcr = defs.add(role.mcr_key, typeid(double));
    mcr->location      = FDMConfigLocation::Print;
    mcr->overrides_in  = {FDMConfigLocation::Tool};
    mcr->category      = ConfigItemDef::Category::Print_MotionDynamics;
    mcr->option_group  = role.option_group;
    mcr->gui_type      = ConfigItemDef::GUIType::textfield;
    mcr->label         = BossL(std::string(role.label_prefix) + " minimum cruise ratio");
    mcr->order         = role.accel_order + 2;
    mcr->full_label    = BossL(std::string(role.tooltip_phrase) + " minimum cruise ratio");
    mcr->tooltip       = BossL(
        "The minimum distance traveled at cruising speed relative to the total "
        "distance traveled before going from acceleration to deceleration for "
        + std::string(role.tooltip_phrase)
        + ". Set to zero to disable control for "
        + std::string(role.tooltip_phrase)
        + " (Klipper only)."
    );
    mcr->min     = 0.0;
    mcr->max     = 0.999999;
    mcr->init_fn = init_with(0.0);

    ConfigItemDef* jerk = defs.add(role.jerk_key, typeid(int));
    jerk->location      = FDMConfigLocation::Print;
    jerk->overrides_in  = {FDMConfigLocation::Tool};
    jerk->category      = ConfigItemDef::Category::Print_MotionDynamics;
    jerk->option_group  = role.option_group;
    jerk->gui_type      = ConfigItemDef::GUIType::spinbox;
    jerk->label         = BossL(std::string(role.label_prefix) + " jerk");
    jerk->order         = role.accel_order + 1;
    jerk->full_label    = BossL(std::string(role.tooltip_phrase) + " jerk");
    jerk->tooltip       = BossL(
        "The jerk your printer will use for "
        + std::string(role.tooltip_phrase)
        + ". Set to zero to disable jerk control for "
        + std::string(role.tooltip_phrase)
        + "."
    );
    jerk->units   = {BossL("mm/s")};
    jerk->min     = 0.0;
    jerk->init_fn = init_with(0);
}

} // namespace

void JerkScvMcrFeature::register_config(Domain::ConfigDefinitions& defs)
{
    for (const RoleSpec& role : kRoles)
        register_role(defs, role);
}

} // namespace Slic3r::Boss
