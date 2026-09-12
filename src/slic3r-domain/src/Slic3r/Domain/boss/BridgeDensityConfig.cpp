///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "boss/features/bridge-density/BridgeDensityFeature.hpp"

#include "boss/foundation/BossL.hpp"
#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefUtils.hpp"

namespace Slic3r::Boss {

void BridgeDensityFeature::register_config(Domain::ConfigDefinitions& defs)
{
    using namespace Slic3r::Domain;

    ConfigItemDef* density = defs.add("bridge_density", typeid(Percentage));
    density->location = FDMConfigLocation::Print;
    density->overrides_in = {FDMConfigLocation::Tool, FDMConfigLocation::Object, FDMConfigLocation::Volume};
    density->label = BossL("Bridge density");
    density->category = ConfigItemDef::Category::Print_Infill;
    density->option_group = ConfigItemDef::OptionGroup::Print_Infill_Advanced;
    density->gui_type = ConfigItemDef::GUIType::textfield;
    density->tooltip = BossL("Density of external bridges. 100% means solid bridge. Default is 100%.\n\n"
                              "Higher densities can produce smoother bridge surfaces, as overlapping lines "
                              "provide additional support during printing. Maximum is 120%. \n"
                              "Note: Bridge density that is too high can cause warping or overextrusion.");
    density->units = {BossL("%")};
    density->min = 10;
    density->max = 120;
    density->init_fn = init_with(Percentage{100.});
}

} // namespace Slic3r::Boss
