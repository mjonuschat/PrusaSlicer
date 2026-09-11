///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
#include "boss/features/z-hop-surface-filter/ZHopSurfaceFilterFeature.hpp"

#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefUtils.hpp"
#include "boss/foundation/BossL.hpp"

namespace Slic3r::Boss {

void ZHopSurfaceFilterFeature::register_config(Domain::ConfigDefinitions& defs)
{
    using namespace Slic3r::Domain;

    ConfigItemDef* def = defs.add("retract_lift_enforce", typeid(EnumWrapper));
    def->location      = FDMConfigLocation::Print;
    def->overrides_in  = {FDMConfigLocation::Tool, FDMConfigLocation::Filament};
    def->category      = ConfigItemDef::Category::Print_MotionDynamics;
    def->option_group  = ConfigItemDef::OptionGroup::Print_MotionDynamics_VerticalLift;
    def->gui_type      = ConfigItemDef::GUIType::i_enum_open;
    def->label         = BossL("Z-hop allowed");
    def->tooltip       = BossL(
        "Restrict Z-hop to specific surface types. \"Everywhere\" applies Z-hop on every "
        "retraction (default behavior). \"Top Surfaces\" only lifts when the nozzle is over "
        "a top surface. \"First Layer\" only lifts on the first layer. \"Top and First "
        "Layer\" lifts in both cases."
    );
    def->init_fn = init_with(
        ZHopSurfaceFilterMode::AllSurfaces,
        EnumValueDefs{
            {static_cast<int>(ZHopSurfaceFilterMode::AllSurfaces),
             "all_surfaces",
             BossL("Everywhere")},
            {static_cast<int>(ZHopSurfaceFilterMode::TopOnly), "top_only", BossL("Top Surfaces")},
            {static_cast<int>(ZHopSurfaceFilterMode::BottomOnly),
             "bottom_only",
             BossL("First Layer")},
            {static_cast<int>(ZHopSurfaceFilterMode::TopAndBottom),
             "top_and_bottom",
             BossL("Top and First Layer")},
        }
    );
}

} // namespace Slic3r::Boss
