///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "boss/features/filament-max-speed/FilamentMaxSpeedFeature.hpp"

#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefUtils.hpp"

namespace Slic3r::Boss {

void FilamentMaxSpeedFeature::register_config(Domain::ConfigDefinitions& defs)
{
    using namespace Slic3r::Domain;

    ConfigItemDef* def = defs.add("filament_max_speed", typeid(double));
    def->location      = FDMConfigLocation::Filament;
    def->category      = ConfigItemDef::Category::Filament_Overrides;
    def->option_group  = ConfigItemDef::OptionGroup::Filament_Overrides_PrintSpeedOverride;
    def->gui_type      = ConfigItemDef::GUIType::textfield;
    def->label         = "Max speed";
    def->tooltip       = "Maximum speed allowed for this filament. Limits the maximum "
                          "speed of a print to the minimum of the print speed and the filament speed. "
                          "Set zero for no limit.";
    def->units         = {"mm/s"};
    def->min           = 0;
    def->init_fn       = init_with(0.);
}

} // namespace Slic3r::Boss
