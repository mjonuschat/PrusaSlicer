///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "boss/features/alternate-extra-perimeter/AlternateExtraPerimeterFeature.hpp"

#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefUtils.hpp"

namespace Slic3r::Boss {

void AlternateExtraPerimeterFeature::register_config(Domain::ConfigDefinitions& defs)
{
    using namespace Slic3r::Domain;

    ConfigItemDef* def = defs.add("alternate_extra_perimeter", typeid(bool));
    def->location      = FDMConfigLocation::Print;
    def->category      = ConfigItemDef::Category::Print_WallsPerimeters;
    def->option_group  = ConfigItemDef::OptionGroup::Print_WallsPerimeters_Perimeters;
    def->gui_type      = ConfigItemDef::GUIType::checkbox;
    def->label         = "Alternate extra wall";
    def->tooltip       = "Add one extra perimeter every other layer. Has no effect when the fill"
                          " density is zero or when spiral vase mode is on.";
    def->init_fn       = init_with(false);
}

} // namespace Slic3r::Boss
