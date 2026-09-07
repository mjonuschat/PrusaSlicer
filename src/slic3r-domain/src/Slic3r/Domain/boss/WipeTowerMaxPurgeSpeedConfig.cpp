///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "boss/features/wipe-tower-max-purge-speed/WipeTowerMaxPurgeSpeedFeature.hpp"

#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefUtils.hpp"
#include "boss/foundation/BossL.hpp"

namespace Slic3r::Boss {

void WipeTowerMaxPurgeSpeedFeature::register_config(Domain::ConfigDefinitions& defs)
{
    using namespace Slic3r::Domain;

    ConfigItemDef* def = defs.add("wipe_tower_max_purge_speed", typeid(double));
    def->location      = FDMConfigLocation::Print;
    def->category      = ConfigItemDef::Category::Print_MultiMaterial;
    def->option_group  = ConfigItemDef::OptionGroup::Print_MultiMaterial_WipeTower;
    def->gui_type      = ConfigItemDef::GUIType::textfield;
    def->label         = BossL("Maximum wipe tower purge speed");
    def->tooltip       = BossL(
        "The maximum speed when purging filament in the wipe tower during a tool change. "
        "Does not affect the wipe tower's other printed sections (sparse layers, "
        "perimeters), which use their own speed settings.");
    def->units         = {BossL("mm/s")};
    def->min           = 10;
    def->init_fn       = init_with(90.);
}

} // namespace Slic3r::Boss
