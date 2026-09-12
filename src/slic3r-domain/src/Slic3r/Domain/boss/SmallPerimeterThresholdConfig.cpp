///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "boss/features/small-perimeter-threshold/SmallPerimeterThresholdFeature.hpp"

#include "boss/foundation/BossL.hpp"
#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefUtils.hpp"

namespace Slic3r::Boss {

void SmallPerimeterThresholdFeature::register_config(Domain::ConfigDefinitions& defs)
{
    using namespace Slic3r::Domain;

    ConfigItemDef* min_length = defs.add("small_perimeter_min_length", typeid(double));
    min_length->location      = FDMConfigLocation::Print;
    min_length->category      = ConfigItemDef::Category::Print_Speed;
    min_length->option_group  = ConfigItemDef::OptionGroup::Print_Speed_MainStructure;
    min_length->gui_type      = ConfigItemDef::GUIType::textfield;
    min_length->label         = BossL("Lower");
    min_length->full_label    = BossL("Lower small perimeter length threshold");
    min_length->tooltip       = BossL(
        "Set the lower threshold for small perimeter length. Every loop with a length "
        "below this value prints at small_perimeter_speed."
    );
    min_length->units         = {BossL("mm")};
    min_length->min           = 0.;
    min_length->max           = 100.;
    min_length->init_fn       = init_with(40.0);

    ConfigItemDef* max_length = defs.add("small_perimeter_max_length", typeid(double));
    max_length->location      = FDMConfigLocation::Print;
    max_length->category      = ConfigItemDef::Category::Print_Speed;
    max_length->option_group  = ConfigItemDef::OptionGroup::Print_Speed_MainStructure;
    max_length->gui_type      = ConfigItemDef::GUIType::textfield;
    max_length->label         = BossL("Upper");
    max_length->full_label    = BossL("Upper small perimeter length threshold");
    max_length->tooltip       = BossL(
        "Set the upper threshold for small perimeter length. Loops with a length between "
        "the lower and upper thresholds get a speed between small_perimeter_speed and the "
        "normal perimeter speed, scaled by length."
    );
    max_length->units         = {BossL("mm")};
    max_length->min           = 0.;
    max_length->max           = 500.;
    max_length->init_fn       = init_with(125.0);
}

} // namespace Slic3r::Boss
