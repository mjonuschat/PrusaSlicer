///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "boss/features/flowsnake/FlowsnakeFeature.hpp"
#include "libslic3r/Fill/FillPlanePath.hpp"

namespace Slic3r::Boss {

std::unique_ptr<Fill> FlowsnakeFeature::create_fill()
{
    return std::make_unique<Slic3r::FillFlowsnake>();
}

bool FlowsnakeFeature::use_bridge_flow()
{
    return false;
}

} // namespace Slic3r::Boss
