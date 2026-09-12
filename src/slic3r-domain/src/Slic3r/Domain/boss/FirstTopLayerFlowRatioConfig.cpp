///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "boss/features/first-top-layer-flow-ratio/FirstTopLayerFlowRatioFeature.hpp"

#include "boss/foundation/BossL.hpp"
#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefUtils.hpp"

namespace Slic3r::Boss {

void FirstTopLayerFlowRatioFeature::register_config(Domain::ConfigDefinitions& defs)
{
    using namespace Slic3r::Domain;

    ConfigItemDef* first_layer = defs.add("first_layer_flow_ratio", typeid(double));
    first_layer->location = FDMConfigLocation::Print;
    first_layer->label = BossL("First layer flow ratio");
    first_layer->category = ConfigItemDef::Category::Print_LayersSurfaces;
    first_layer->option_group = ConfigItemDef::OptionGroup::Print_LayerSurfaces_LayerHeight;
    first_layer->gui_type = ConfigItemDef::GUIType::textfield;
    first_layer->tooltip = BossL(
        "This factor affects the amount of plastic for first layer. You can decrease it slightly "
        "(e.g. 0.85) to prevent rough first layer and sticking to the nozzle, or increase a bit to "
        "improve sticking to unflat bed (though it's better to have autoleveling or flat bed)."
    );
    first_layer->min = 0.5;
    first_layer->max = 1.5;
    first_layer->init_fn = init_with(1.);

    ConfigItemDef* top_layer = defs.add("top_layer_flow_ratio", typeid(double));
    top_layer->location = FDMConfigLocation::Print;
    top_layer->label = BossL("Top layer flow ratio");
    top_layer->category = ConfigItemDef::Category::Print_LayersSurfaces;
    top_layer->option_group = ConfigItemDef::OptionGroup::Print_LayerSurfaces_TopBottomShells;
    top_layer->gui_type = ConfigItemDef::GUIType::textfield;
    top_layer->tooltip = BossL(
        "This factor affects the amount of plastic for top layer. Play with this parameter to get "
        "a smooth surface."
    );
    top_layer->min = 0.5;
    top_layer->max = 1.5;
    top_layer->init_fn = init_with(1.);
}

} // namespace Slic3r::Boss
