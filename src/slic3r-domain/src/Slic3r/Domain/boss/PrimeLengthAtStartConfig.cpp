///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "boss/features/prime-length-at-start/PrimeLengthAtStartFeature.hpp"

#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefUtils.hpp"

namespace Slic3r::Boss {

void PrimeLengthAtStartFeature::register_config(Domain::ConfigDefinitions& defs)
{
    using namespace Slic3r::Domain;

    ConfigItemDef* def = defs.add("prime_length_at_start", typeid(double));
    def->location      = FDMConfigLocation::Filament;
    def->category      = ConfigItemDef::Category::Filament_Overrides;
    def->option_group  = ConfigItemDef::OptionGroup::Filament_Overrides_PrintSpeedOverride;
    def->gui_type      = ConfigItemDef::GUIType::textfield;
    def->label         = "Prime length at start";
    def->tooltip       = "Length of filament to extrude before the first perimeter of the print. "
                          "Use this to prime the nozzle after loading filament. Set zero to disable.";
    def->units         = {"mm (zero to disable)"};
    def->min           = 0;
    def->init_fn       = init_with(0.);
}

} // namespace Slic3r::Boss
