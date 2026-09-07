///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "boss/features/per-object-multiplier/PerObjectMultiplierFeature.hpp"

#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefUtils.hpp"

namespace Slic3r::Boss {

void PerObjectMultiplierFeature::register_config(Domain::ConfigDefinitions& defs)
{
    using namespace Slic3r::Domain;

    ConfigItemDef* def = defs.add("print_extrusion_multiplier", typeid(double));
    def->location      = FDMConfigLocation::Print;
    def->overrides_in  = std::set<ConfigLocation>{FDMConfigLocation::Object};
    def->category      = ConfigItemDef::Category::Print_ExtrusionRetraction;
    def->option_group  = ConfigItemDef::OptionGroup::Print_ExtrusionRetraction_ExtrusionWidth;
    def->gui_type      = ConfigItemDef::GUIType::textfield;
    def->label         = "Extrusion multiplier";
    def->tooltip       = "This factor changes the amount of flow proportionally. You may need to tweak "
                          "this setting to get nice surface finish and correct single wall widths. "
                          "Usual values are between 90% and 110%. This print setting is multiplied "
                          "with the extrusion_multiplier from the filament tab. Its only purpose is to "
                          "offer the same functionality but on a per-object basis.";
    def->min           = 0.5;
    def->max           = 1.5;
    def->init_fn       = init_with(1.);
}

} // namespace Slic3r::Boss
