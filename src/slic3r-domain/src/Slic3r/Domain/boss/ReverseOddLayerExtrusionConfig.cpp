///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/ Copyright (c) OrcaSlicer 2023 Noisyfox @Noisyfox
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "boss/features/reverse-odd-layer-extrusion/ReverseOddLayerExtrusionFeature.hpp"

#include "boss/foundation/BossL.hpp"
#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefUtils.hpp"

namespace Slic3r::Boss {

void ReverseOddLayerExtrusionFeature::register_config(Domain::ConfigDefinitions& defs)
{
    using namespace Slic3r::Domain;

    ConfigItemDef* internal_perimeters = defs.add("internal_perimeters_reverse", typeid(bool));
    internal_perimeters->location      = FDMConfigLocation::Print;
    internal_perimeters->category      = ConfigItemDef::Category::Print_WallsPerimeters;
    internal_perimeters->option_group  = ConfigItemDef::OptionGroup::Print_WallsPerimeters_Perimeters;
    internal_perimeters->gui_type      = ConfigItemDef::GUIType::checkbox;
    internal_perimeters->label         = BossL("For internal perimeters");
    internal_perimeters->full_label    = BossL("Reverse on odd layers: Internal perimeters");
    internal_perimeters->tooltip       = BossL(
        "Extrude internal perimeters in the opposite direction on odd layers. This "
        "reduces stress and warping."
    );
    internal_perimeters->init_fn       = init_with(false);

    ConfigItemDef* overhangs = defs.add("overhangs_reverse", typeid(bool));
    overhangs->location     = FDMConfigLocation::Print;
    overhangs->category     = ConfigItemDef::Category::Print_WallsPerimeters;
    overhangs->option_group = ConfigItemDef::OptionGroup::Print_WallsPerimeters_Perimeters;
    overhangs->gui_type     = ConfigItemDef::GUIType::checkbox;
    overhangs->label        = BossL("For overhang perimeters");
    overhangs->full_label   = BossL("Reverse on odd layers: Overhangs");
    overhangs->tooltip      = BossL(
        "Extrude perimeters that touch an overhang in the opposite direction on odd "
        "layers. This improves steep overhangs."
    );
    overhangs->init_fn      = init_with(false);

    ConfigItemDef* infill = defs.add("infill_reverse", typeid(bool));
    infill->location     = FDMConfigLocation::Print;
    infill->category     = ConfigItemDef::Category::Print_Infill;
    infill->option_group = ConfigItemDef::OptionGroup::Print_Infill_Advanced;
    infill->gui_type     = ConfigItemDef::GUIType::checkbox;
    infill->label        = BossL("For infill");
    infill->full_label   = BossL("Reverse on odd layers: Infill");
    infill->tooltip      = BossL(
        "Extrude infill in the opposite direction on odd layers. This reduces stress "
        "and warping."
    );
    infill->init_fn      = init_with(false);
}

} // namespace Slic3r::Boss
