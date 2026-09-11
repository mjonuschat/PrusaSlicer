///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "boss/features/small-area-flow-compensation/SmallAreaFlowCompensationFeature.hpp"

#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefUtils.hpp"
#include "boss/foundation/BossL.hpp"

namespace Slic3r::Boss {

namespace {

constexpr double kDefaultLengths[10] = {0, 0.2, 0.4, 0.6, 0.8, 1.5, 2, 3, 5, 10};
constexpr double kDefaultFactors[10] = {0, 0.4444, 0.6145, 0.7059, 0.7619, 0.8571, 0.8889, 0.9231, 0.9520, 1.0};

void register_point(Domain::ConfigDefinitions &defs, int index)
{
    using namespace Slic3r::Domain;

    ConfigItemDef *length = defs.add(
        "small_area_infill_flow_compensation_extrusion_length_" + std::to_string(index), typeid(double)
    );
    length->location     = FDMConfigLocation::Print;
    length->category     = ConfigItemDef::Category::Print_Infill;
    length->option_group = ConfigItemDef::OptionGroup::Print_Infill_Advanced;
    length->gui_type     = ConfigItemDef::GUIType::textfield;
    length->label        = BossL("Extrusion length");
    length->tooltip      = BossL(
        "Extrusion length up to which the flow compensation applies. Typical range is 0-20mm."
    );
    length->units   = {BossL("mm")};
    length->min     = 0.0;
    length->max     = 100.0;
    length->init_fn = init_with(kDefaultLengths[index]);

    ConfigItemDef *factor = defs.add(
        "small_area_infill_flow_compensation_compensation_factor_" + std::to_string(index), typeid(double)
    );
    factor->location     = FDMConfigLocation::Print;
    factor->category     = ConfigItemDef::Category::Print_Infill;
    factor->option_group = ConfigItemDef::OptionGroup::Print_Infill_Advanced;
    factor->gui_type     = ConfigItemDef::GUIType::textfield;
    factor->label        = BossL("Compensation factor");
    factor->tooltip      = BossL("Compensation factor to apply to the extrusion amount.");
    factor->min          = 0.0;
    factor->max          = 1.0;
    factor->init_fn      = init_with(kDefaultFactors[index]);
}

} // namespace

void SmallAreaFlowCompensationFeature::register_config(Domain::ConfigDefinitions &defs)
{
    using namespace Slic3r::Domain;

    ConfigItemDef *enable = defs.add("small_area_infill_flow_compensation", typeid(bool));
    enable->location      = FDMConfigLocation::Print;
    enable->category      = ConfigItemDef::Category::Print_Infill;
    enable->option_group  = ConfigItemDef::OptionGroup::Print_Infill_Advanced;
    enable->gui_type      = ConfigItemDef::GUIType::checkbox;
    enable->label         = BossL("Enable small area infill flow compensation");
    enable->tooltip       = BossL("Enable flow compensation for small infill areas.");
    enable->init_fn       = init_with(false);

    for (int i = 0; i < 10; ++i)
        register_point(defs, i);
}

} // namespace Slic3r::Boss
