///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "boss/features/autoemit-toolchange-commands/AutoemitToolchangeCommandsFeature.hpp"

#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefUtils.hpp"

namespace Slic3r::Boss {

void AutoemitToolchangeCommandsFeature::register_config(Domain::ConfigDefinitions& defs)
{
    using namespace Slic3r::Domain;

    ConfigItemDef* def = defs.add("autoemit_toolchange_commands", typeid(bool));
    def->location      = FDMConfigLocation::Printer;
    def->category      = ConfigItemDef::Category::Printer_CustomGCode;
    def->option_group  = ConfigItemDef::OptionGroup::Printer_CustomGCode;
    def->gui_type      = ConfigItemDef::GUIType::checkbox;
    def->label         = "Automatically emit tool-change commands";
    def->tooltip       = "If enabled, PrusaSlicer emits an automatic tool-change command after the "
                          "tool-change G-code, unless the tool-change G-code already contains one. "
                          "If disabled, PrusaSlicer never emits an automatic tool-change command, so "
                          "the tool-change G-code must issue it.";
    def->init_fn       = init_with(true);
}

} // namespace Slic3r::Boss
