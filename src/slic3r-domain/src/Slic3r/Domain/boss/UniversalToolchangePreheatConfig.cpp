///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "boss/features/universal-toolchange-preheat/UniversalToolchangePreheatFeature.hpp"

#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefUtils.hpp"

namespace Slic3r::Boss {

void UniversalToolchangePreheatFeature::register_config(Domain::ConfigDefinitions& defs)
{
    using namespace Slic3r::Domain;

    ConfigItemDef* time = defs.add("preheat_time", typeid(double));
    time->location      = FDMConfigLocation::Print;
    time->category      = ConfigItemDef::Category::Print_MultiMaterial;
    time->option_group  = ConfigItemDef::OptionGroup::Print_MultiMaterial_OozePrevention;
    time->order         = 1;
    time->gui_type      = ConfigItemDef::GUIType::textfield;
    time->label         = "Preheat time";
    time->tooltip       = "Time before a tool change during which PrusaSlicer sends preheat commands "
                           "for the next tool. Set to 0 to turn the preheat off.";
    time->units         = {"s"};
    time->min           = 0;
    time->init_fn       = init_with(120.);

    ConfigItemDef* steps = defs.add("preheat_steps", typeid(int));
    steps->location      = FDMConfigLocation::Print;
    steps->category      = ConfigItemDef::Category::Print_MultiMaterial;
    steps->option_group  = ConfigItemDef::OptionGroup::Print_MultiMaterial_OozePrevention;
    steps->order         = 2;
    steps->gui_type      = ConfigItemDef::GUIType::spinbox;
    steps->label         = "Preheat steps";
    steps->tooltip       = "Number of preheat commands PrusaSlicer sends during the preheat time "
                            "before a tool change. Only a printer that reheats in steps, such as the "
                            "Prusa XL, needs more than one. Set this to 1 for other printers.";
    steps->min           = 1;
    steps->init_fn       = init_with(10);
}

} // namespace Slic3r::Boss
