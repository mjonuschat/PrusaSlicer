///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "boss/features/alternate-extra-perimeter/AlternateExtraPerimeterFeature.hpp"

#include "libslic3r/ConfigViews.hpp"
#include "libslic3r/boss/perimeter/PerimeterPolicyContext.hpp"

namespace Slic3r::Boss {

int AlternateExtraPerimeterFeature::adjust_loop_count(int loop_count, const PerimeterPolicyContext& ctx)
{
    if (ctx.config->get<bool>("alternate_extra_perimeter") &&
        ctx.layer_id % 2 == 1 && !ctx.spiral_vase && ctx.fill_density > 0.0)
        return loop_count + 1;
    return loop_count;
}

} // namespace Slic3r::Boss
