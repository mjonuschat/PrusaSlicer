///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "boss/features/z-rotate/ZRotateFeature.hpp"

#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefUtils.hpp"

namespace Slic3r::Boss {

void ZRotateFeature::register_config(Domain::ConfigDefinitions& defs)
{
    using namespace Slic3r::Domain;

    ConfigItemDef* def = defs.add("init_z_rotate", typeid(double));
    def->location      = FDMConfigLocation::Printer;
    def->category      = ConfigItemDef::Category::Printer_General;
    def->option_group  = ConfigItemDef::OptionGroup::Printer_General_Advanced;
    def->gui_type      = ConfigItemDef::GUIType::spinbox;
    def->label         = "Preferred Z rotation";
    def->tooltip       = "Rotate objects around the Z axis while adding them to the bed, "
                          "in degrees.";
    def->units         = {"°"};
    def->min           = -360.;
    def->max           = 360.;
    def->init_fn       = init_with(0.);
}

} // namespace Slic3r::Boss
