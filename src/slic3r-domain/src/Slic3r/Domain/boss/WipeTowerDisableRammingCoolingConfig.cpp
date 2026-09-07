///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "boss/features/wipe-tower-disable-ramming-cooling/WipeTowerDisableRammingCoolingFeature.hpp"

#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefUtils.hpp"

namespace Slic3r::Boss {

void WipeTowerDisableRammingCoolingFeature::register_config(Domain::ConfigDefinitions& defs)
{
    using namespace Slic3r::Domain;

    ConfigItemDef* ramming = defs.add("wipe_tower_disable_filament_ramming", typeid(bool));
    ramming->location      = FDMConfigLocation::Print;
    ramming->category      = ConfigItemDef::Category::Print_MultiMaterial;
    ramming->option_group  = ConfigItemDef::OptionGroup::Print_MultiMaterial_WipeTower;
    ramming->gui_type      = ConfigItemDef::GUIType::checkbox;
    ramming->label         = "Disable all filament ramming";
    ramming->tooltip       = "If enabled, all filament ramming will be disabled, and the per-filament "
                              "configuration options will be ignored.";
    ramming->init_fn       = init_with(false);

    ConfigItemDef* cooling = defs.add("wipe_tower_disable_cooling_moves", typeid(bool));
    cooling->location      = FDMConfigLocation::Print;
    cooling->category      = ConfigItemDef::Category::Print_MultiMaterial;
    cooling->option_group  = ConfigItemDef::OptionGroup::Print_MultiMaterial_WipeTower;
    cooling->gui_type      = ConfigItemDef::GUIType::checkbox;
    cooling->label         = "Disable all cooling moves";
    cooling->tooltip       = "If enabled, all filament cooling moves will be disabled, and the "
                              "per-filament configuration options will be ignored.";
    cooling->init_fn       = init_with(false);
}

} // namespace Slic3r::Boss
