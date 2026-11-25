///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
#include "boss/features/aligned-rear-seam/AlignedRearSeamFeature.hpp"
#include "boss/foundation/BossL.hpp"
#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefsFDM.hpp"
#include "Slic3r/Domain/ConfigDefUtils.hpp"

namespace Slic3r::Boss {

void AlignedRearSeamFeature::register_config(Domain::ConfigDefinitions &defs)
{
    using namespace Slic3r::Domain;

    // A choice, not an option of its own: BOSS 2.9.x offered it as a fifth seam
    // position and that is where users look for it.
    append_enum_choice(
        defs,
        "seam_position",
        int(SeamPosition::spAlignedRear),
        "aligned_rear",
        BossL("Aligned Rear")
    );
}

} // namespace Slic3r::Boss
