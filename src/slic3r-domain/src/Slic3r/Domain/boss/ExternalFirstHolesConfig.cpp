///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "boss/features/external-first-holes/ExternalFirstHolesFeature.hpp"

#include "boss/foundation/BossL.hpp"
#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefUtils.hpp"

namespace Slic3r::Boss {

void ExternalFirstHolesFeature::register_config(Domain::ConfigDefinitions& defs)
{
    using namespace Slic3r::Domain;

    // Mirrors the native external_perimeters_first bool's cardinality: a
    // per-extruder override, resolved with .at(extruder_id) at the call site.
    ConfigItemDef* holes = defs.add("external_perimeters_first_holes", typeid(bool));
    holes->location      = FDMConfigLocation::Print;
    holes->overrides_in  = {FDMConfigLocation::Tool, FDMConfigLocation::Object, FDMConfigLocation::Volume};
    holes->category      = ConfigItemDef::Category::Print_WallsPerimeters;
    holes->option_group  = ConfigItemDef::OptionGroup::Print_WallsPerimeters_WallsQuality;
    holes->gui_type      = ConfigItemDef::GUIType::checkbox;
    holes->label         = BossL("Holes");
    holes->full_label    = BossL("External perimeters first for holes");
    holes->tooltip       = BossL("Print hole perimeters from the outermost one to the innermost one "
                                  "instead of the default inverse order.");
    holes->init_fn       = init_with(true);

    ConfigItemDef* min_size = defs.add("external_perimeters_first_holes_min_size", typeid(double));
    min_size->location     = FDMConfigLocation::Print;
    min_size->category     = ConfigItemDef::Category::Print_WallsPerimeters;
    min_size->option_group = ConfigItemDef::OptionGroup::Print_WallsPerimeters_WallsQuality;
    min_size->gui_type     = ConfigItemDef::GUIType::textfield;
    min_size->label        = BossL("Minimum hole size");
    min_size->full_label   = BossL("External perimeters first minimum hole size");
    min_size->tooltip      = BossL("The minimum length of the hole perimeter needed to enable external "
                                    "perimeters first for holes.");
    min_size->units        = {BossL("mm")};
    min_size->min          = 0.;
    min_size->init_fn      = init_with(30.0);

    ConfigItemDef* disabled_first_layers = defs.add("external_perimeters_first_disabled_first_layers", typeid(int));
    disabled_first_layers->location     = FDMConfigLocation::Print;
    disabled_first_layers->category     = ConfigItemDef::Category::Print_WallsPerimeters;
    disabled_first_layers->option_group = ConfigItemDef::OptionGroup::Print_WallsPerimeters_WallsQuality;
    disabled_first_layers->gui_type     = ConfigItemDef::GUIType::spinbox;
    disabled_first_layers->label        = BossL("External perimeters first disabled for first");
    disabled_first_layers->full_label   = BossL("External perimeters first disabled for first layers");
    disabled_first_layers->tooltip      = BossL("You can set this to a positive value to disable external "
                                                 "perimeters first for the first layers, so that it does "
                                                 "not affect fillets/chamfers.");
    disabled_first_layers->units        = {BossL("layers")};
    disabled_first_layers->min          = 0;
    disabled_first_layers->max          = 1000;
    disabled_first_layers->init_fn      = init_with(0);
}

} // namespace Slic3r::Boss
