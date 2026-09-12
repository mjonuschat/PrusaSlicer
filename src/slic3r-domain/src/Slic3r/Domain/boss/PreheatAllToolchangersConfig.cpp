///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "boss/features/preheat-all-toolchangers/PreheatAllToolchangersFeature.hpp"

#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefUtils.hpp"

namespace Slic3r::Boss {

void PreheatAllToolchangersFeature::register_config(Domain::ConfigDefinitions& defs)
{
    using namespace Slic3r::Domain;

    ConfigItemDef* toolchangers = defs.add("preheat_toolchangers", typeid(bool));
    toolchangers->location      = FDMConfigLocation::Print;
    toolchangers->category      = ConfigItemDef::Category::Print_MultiMaterial;
    toolchangers->option_group  = ConfigItemDef::OptionGroup::Print_MultiMaterial_ToolChanges;
    toolchangers->gui_type      = ConfigItemDef::GUIType::checkbox;
    toolchangers->label         = "Preheat all toolchangers";
    toolchangers->tooltip       = "If enabled, PrusaSlicer emits firmware preheat commands (M104.1) for "
                                   "every tool change, even if the printer profile does not declare "
                                   "support for tool preheating.";
    toolchangers->init_fn       = init_with(false);

    ConfigItemDef* time = defs.add("preheat_time", typeid(double));
    time->location      = FDMConfigLocation::Print;
    time->category      = ConfigItemDef::Category::Print_MultiMaterial;
    time->option_group  = ConfigItemDef::OptionGroup::Print_MultiMaterial_ToolChanges;
    time->gui_type      = ConfigItemDef::GUIType::textfield;
    time->label         = "Preheat time";
    time->tooltip       = "Time before a tool change during which PrusaSlicer sends preheat commands "
                           "for the next tool.";
    time->units         = {"s"};
    time->min           = 0;
    time->init_fn       = init_with(120.);

    ConfigItemDef* steps = defs.add("preheat_steps", typeid(int));
    steps->location      = FDMConfigLocation::Print;
    steps->category      = ConfigItemDef::Category::Print_MultiMaterial;
    steps->option_group  = ConfigItemDef::OptionGroup::Print_MultiMaterial_ToolChanges;
    steps->gui_type      = ConfigItemDef::GUIType::spinbox;
    steps->label         = "Preheat steps";
    steps->tooltip       = "Number of preheat commands PrusaSlicer sends during the preheat time "
                            "before a tool change.";
    steps->min           = 1;
    steps->init_fn       = init_with(10);
}

} // namespace Slic3r::Boss
