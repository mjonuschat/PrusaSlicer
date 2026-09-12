///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "boss/features/narrow-solid-infill-erosion/NarrowSolidInfillErosionFeature.hpp"

#include "boss/foundation/BossL.hpp"
#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefUtils.hpp"

namespace Slic3r::Boss {

void NarrowSolidInfillErosionFeature::register_config(Domain::ConfigDefinitions& defs)
{
    using namespace Slic3r::Domain;

    ConfigItemDef* detect = defs.add("detect_narrow_solid_infill", typeid(bool));
    detect->location     = FDMConfigLocation::Print;
    detect->overrides_in = {FDMConfigLocation::Object, FDMConfigLocation::Volume};
    detect->category     = ConfigItemDef::Category::Print_LayersSurfaces;
    detect->option_group = ConfigItemDef::OptionGroup::Print_LayerSurfaces_SurfacePatterns;
    detect->gui_type     = ConfigItemDef::GUIType::checkbox;
    detect->label        = BossL("Detect narrow solid infill");
    detect->tooltip      = BossL("If enabled, narrow internal solid infill surfaces are filled using "
                                  "variable-width paths (Arachne) instead of the configured pattern, which "
                                  "prevents zigzag artifacts and unfilled gaps on thin solid areas. Does not "
                                  "affect top, bottom, or bridge infill.");
    detect->init_fn      = init_with(true);

    ConfigItemDef* threshold = defs.add("detect_narrow_solid_infill_threshold", typeid(double));
    threshold->location     = FDMConfigLocation::Print;
    threshold->overrides_in = {FDMConfigLocation::Object, FDMConfigLocation::Volume};
    threshold->category     = ConfigItemDef::Category::Print_LayersSurfaces;
    threshold->option_group = ConfigItemDef::OptionGroup::Print_LayerSurfaces_SurfacePatterns;
    threshold->gui_type     = ConfigItemDef::GUIType::textfield;
    threshold->label        = BossL("Narrow threshold");
    threshold->tooltip      = BossL("Solid infill areas narrower than this many extrusion widths will use "
                                     "variable-width fill instead of the configured pattern. Only applies when "
                                     "'Detect narrow solid infill' is enabled.");
    threshold->units        = {BossL("x extrusion width")};
    threshold->min          = 1.0;
    threshold->max          = 10.0;
    threshold->init_fn      = init_with(3.0);
}

} // namespace Slic3r::Boss
