///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "boss/features/perimeter-overlap/PerimeterOverlapFeature.hpp"

#include "boss/foundation/BossL.hpp"
#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefUtils.hpp"

namespace Slic3r::Boss {

void PerimeterOverlapFeature::register_config(Domain::ConfigDefinitions& defs)
{
    using namespace Slic3r::Domain;

    ConfigItemDef* external = defs.add("external_perimeter_overlap", typeid(FloatOrPercentage));
    external->location      = FDMConfigLocation::Print;
    external->category      = ConfigItemDef::Category::Print_WallsPerimeters;
    external->option_group  = ConfigItemDef::OptionGroup::Print_WallsPerimeters_Perimeters;
    external->gui_type      = ConfigItemDef::GUIType::unit_or_percentage;
    external->label         = BossL("Ext. perimeter/perimeter overlap");
    external->tooltip       = BossL(
        "Set the overlap between the external perimeter and the first internal perimeter. "
        "Extruded beads have rounded edges. The beads must overlap for good bonding. "
        "The default value, 10.73%, comes from the constant (1 - pi/4) / 2. This constant "
        "accounts for the round shape of the bead cross-section. The overlap amount scales "
        "with the layer height. At 10.73%, beads bond well. At 100%, beads overlap "
        "completely. This option needs at least 2 perimeters to have an effect."
    );
    external->units    = {BossL("mm"), BossL("%")};
    external->ratio_over = "layer_height";
    external->init_fn = init_with(FloatOrPercentage(Percentage{10.73}));

    ConfigItemDef* internal = defs.add("perimeter_perimeter_overlap", typeid(FloatOrPercentage));
    internal->location      = FDMConfigLocation::Print;
    internal->category      = ConfigItemDef::Category::Print_WallsPerimeters;
    internal->option_group  = ConfigItemDef::OptionGroup::Print_WallsPerimeters_Perimeters;
    internal->gui_type      = ConfigItemDef::GUIType::unit_or_percentage;
    internal->label         = BossL("Perimeter/perimeter overlap");
    internal->tooltip       = BossL(
        "Set the overlap between internal perimeters. Extruded beads have rounded edges. "
        "The beads must overlap for good bonding. The default value, 10.73%, comes from the "
        "constant (1 - pi/4) / 2. This constant accounts for the round shape of the bead "
        "cross-section. The overlap amount scales with the layer height. At 10.73%, beads "
        "bond well. At 100%, beads overlap completely. This option needs at least 3 "
        "perimeters to have an effect. The maximum value is 80%."
    );
    internal->units    = {BossL("mm"), BossL("%")};
    internal->ratio_over = "layer_height";
    internal->init_fn = init_with(FloatOrPercentage(Percentage{10.73}));
}

} // namespace Slic3r::Boss
