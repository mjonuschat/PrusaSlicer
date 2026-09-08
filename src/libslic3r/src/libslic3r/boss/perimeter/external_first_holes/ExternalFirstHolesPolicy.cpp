///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "boss/features/external-first-holes/ExternalFirstHolesFeature.hpp"

#include <vector>

#include "libslic3r/ConfigViews.hpp"
#include "libslic3r/boss/perimeter/PerimeterPolicyContext.hpp"

namespace Slic3r::Boss {

void ExternalFirstHolesFeature::apply_ordering(OrderingPolicy& policy, const PerimeterPolicyContext& ctx)
{
    const int disabled_first_layers = ctx.config->get<int>("external_perimeters_first_disabled_first_layers");

    // Mirrors BOSS's reverse_layer gate: within the disabled window, contours
    // fall back to the pre-BOSS default ordering too, not just holes.
    if (ctx.layer_id < disabled_first_layers) {
        policy.contours_external_first = false;
        return;
    }

    policy.holes_external_first =
        ctx.config->get<std::vector<bool>>("external_perimeters_first_holes").at(ctx.extruder_id);
    policy.min_hole_perimeter_length = ctx.config->get<double>("external_perimeters_first_holes_min_size");
}

} // namespace Slic3r::Boss
