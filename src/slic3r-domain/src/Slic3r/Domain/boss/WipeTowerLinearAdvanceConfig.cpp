///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "boss/features/wipe-tower-linear-advance/WipeTowerLinearAdvanceFeature.hpp"

#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefUtils.hpp"

namespace Slic3r::Boss {

void WipeTowerLinearAdvanceFeature::register_config(Domain::ConfigDefinitions& defs)
{
    using namespace Slic3r::Domain;

    ConfigItemDef* linear_advance = defs.add("wipe_tower_disable_linear_advance", typeid(bool));
    linear_advance->location      = FDMConfigLocation::Print;
    linear_advance->category      = ConfigItemDef::Category::Print_MultiMaterial;
    linear_advance->option_group  = ConfigItemDef::OptionGroup::Print_MultiMaterial_WipeTower;
    linear_advance->gui_type      = ConfigItemDef::GUIType::checkbox;
    linear_advance->label         = "Disable linear advance during wipe tower purge";
    linear_advance->tooltip       = "If enabled, linear advance (pressure advance) is suppressed at both "
                                     "wipe tower ramming and cooling moves, regardless of the printer's "
                                     "pressure advance during ramming setting.";
    linear_advance->init_fn       = init_with(true);
}

} // namespace Slic3r::Boss
