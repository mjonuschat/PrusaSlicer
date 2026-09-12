///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
#include "boss/features/aligned-rear-seam/AlignedRearSeamFeature.hpp"
#include "boss/foundation/BossL.hpp"
#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefUtils.hpp"

namespace Slic3r::Boss {

void AlignedRearSeamFeature::register_config(Domain::ConfigDefinitions &defs)
{
    using namespace Slic3r::Domain;

    ConfigItemDef *def = defs.add("seam_position_aligned_rear", typeid(bool));
    def->location     = FDMConfigLocation::Print;
    def->category     = ConfigItemDef::Category::Print_WallsPerimeters;
    def->option_group = ConfigItemDef::OptionGroup::Print_WallsPerimeters_Seams;
    def->gui_type      = ConfigItemDef::GUIType::checkbox;
    def->label         = BossL("Bias aligned seam to the back");
    def->tooltip       = BossL("Effective only when Seam position is Aligned. Biases the "
                                "visibility search toward the back of the model, similar to "
                                "Rear. Aligned's cross-layer smoothing still applies.");
    def->init_fn       = init_with(false);
}

} // namespace Slic3r::Boss
