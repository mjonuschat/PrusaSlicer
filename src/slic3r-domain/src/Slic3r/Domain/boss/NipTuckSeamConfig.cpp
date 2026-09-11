///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
#include "boss/features/nip-tuck-seam/NipTuckSeamFeature.hpp"
#include "boss/foundation/BossL.hpp"
#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefUtils.hpp"

namespace Slic3r::Boss {

void NipTuckSeamFeature::register_config(Domain::ConfigDefinitions &defs)
{
    using namespace Slic3r::Domain;

    ConfigItemDef *type = defs.add("seam_type", typeid(EnumWrapper));
    type->location     = FDMConfigLocation::Print;
    type->category     = ConfigItemDef::Category::Print_WallsPerimeters;
    type->option_group = ConfigItemDef::OptionGroup::Print_WallsPerimeters_Seams;
    type->gui_type      = ConfigItemDef::GUIType::combobox;
    type->label         = BossL("Nip/Tuck seams");
    type->tooltip       = BossL(
        "Creates a small V-shaped channel at the seam point on external perimeters to hide "
        "start/stop blobs. The external perimeter is nipped inward at the seam and the first "
        "inner perimeter is tucked to absorb the disturbance, so remaining perimeters are "
        "unaffected. Requires at least 2 perimeters. Skipped on single-perimeter regions and "
        "in spiral vase mode."
    );
    type->init_fn = init_with(
        NipTuckSeamType::Regular,
        EnumValueDefs{
            {static_cast<int>(NipTuckSeamType::Regular), "regular", BossL("Regular")},
            {static_cast<int>(NipTuckSeamType::NipTuck), "niptuck", BossL("Nip/Tuck")},
            {static_cast<int>(NipTuckSeamType::Nip), "nip", BossL("Nip")},
            {static_cast<int>(NipTuckSeamType::Tuck), "tuck", BossL("Tuck")},
            {static_cast<int>(NipTuckSeamType::Alternating), "alternating", BossL("Alternating")},
        }
    );

    ConfigItemDef *width = defs.add("seam_notch_width", typeid(double));
    width->location     = FDMConfigLocation::Print;
    width->category     = ConfigItemDef::Category::Print_WallsPerimeters;
    width->option_group = ConfigItemDef::OptionGroup::Print_WallsPerimeters_Seams;
    width->gui_type      = ConfigItemDef::GUIType::textfield;
    width->label         = BossL("Nip/Tuck width");
    width->tooltip       = BossL(
        "Width of the V-shaped notch as a multiple of the external perimeter extrusion width. "
        "The depth is automatically calculated from perimeter spacing."
    );
    width->units         = {BossL("x ext. width")};
    width->min           = 1.0;
    width->max           = 3.0;
    width->init_fn       = init_with(2.0);

    ConfigItemDef *angle = defs.add("seam_notch_angle", typeid(double));
    angle->location     = FDMConfigLocation::Print;
    angle->category     = ConfigItemDef::Category::Print_WallsPerimeters;
    angle->option_group = ConfigItemDef::OptionGroup::Print_WallsPerimeters_Seams;
    angle->gui_type      = ConfigItemDef::GUIType::textfield;
    angle->label         = BossL("Nip/Tuck corner threshold");
    angle->tooltip       = BossL(
        "Seams on corners sharper than this angle are skipped, since a sharp corner already "
        "hides the seam on its own. Set to 0 to apply Nip/Tuck everywhere."
    );
    angle->units         = {BossL("°")};
    angle->min           = 0.0;
    angle->max           = 90.0;
    angle->init_fn       = init_with(44.0);
}

} // namespace Slic3r::Boss
