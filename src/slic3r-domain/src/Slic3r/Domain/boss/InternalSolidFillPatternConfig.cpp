///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/ Copyright (c) SuperSlicer 2019 Remi Durand @supermerill
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "boss/features/internal-solid-fill-pattern/InternalSolidFillPatternFeature.hpp"

#include "boss/foundation/BossL.hpp"
#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefUtils.hpp"
#include "Slic3r/Domain/ConfigDefsFDM.hpp"

namespace Slic3r::Boss {

void InternalSolidFillPatternFeature::register_config(Domain::ConfigDefinitions& defs)
{
    using namespace Slic3r::Domain;

    ConfigItemDef* def = defs.add("solid_fill_pattern", typeid(EnumWrapper));
    def->location     = FDMConfigLocation::Print;
    def->overrides_in = {FDMConfigLocation::Object, FDMConfigLocation::Volume};
    def->label        = BossL("Solid fill pattern");
    def->option_group = ConfigItemDef::OptionGroup::Print_LayerSurfaces_SurfacePatterns;
    def->category     = ConfigItemDef::Category::Print_LayersSurfaces;
    def->order        = 2;
    def->gui_type     = ConfigItemDef::GUIType::combobox;
    def->tooltip      = BossL("Fill pattern for solid (internal) infill. This only affects the solid, "
                               "not-visible layers. Use rectilinear in most cases.");
    def->cli          = "solid-fill-pattern";
    def->init_fn      = init_with(
        InfillPattern::ipMonotonic,
        {{int(InfillPattern::ipRectilinear), "rectilinear", BossL("Rectilinear")},
         {int(InfillPattern::ipMonotonic), "monotonic", BossL("Monotonic")},
         {int(InfillPattern::ipMonotonicLines), "monotoniclines", BossL("Monotonic Lines")},
         {int(InfillPattern::ipAlignedRectilinear), "alignedrectilinear", BossL("Aligned Rectilinear")},
         {int(InfillPattern::ipConcentric), "concentric", BossL("Concentric")},
         {int(InfillPattern::ipHilbertCurve), "hilbertcurve", BossL("Hilbert Curve")},
         {int(InfillPattern::ipArchimedeanChords), "archimedeanchords", BossL("Archimedean Chords")},
         {int(InfillPattern::ipOctagramSpiral), "octagramspiral", BossL("Octagram Spiral")}}
    );
}

} // namespace Slic3r::Boss
