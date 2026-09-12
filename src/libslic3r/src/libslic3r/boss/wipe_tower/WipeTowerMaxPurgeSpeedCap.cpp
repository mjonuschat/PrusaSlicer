///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "boss/features/wipe-tower-max-purge-speed/WipeTowerMaxPurgeSpeedFeature.hpp"

#include "libslic3r/ConfigViews.hpp"

namespace Slic3r::Boss {

std::optional<float> WipeTowerMaxPurgeSpeedFeature::speed_cap(
    const Slic3r::PrintConfigView& config,
    const std::vector<unsigned>& /*extruder_candidates*/
)
{
    // The option's schema enforces a minimum of 10, so this is always a real
    // cap -- unlike filament-max-speed, there is no "0 means unset" case here.
    return static_cast<float>(config.get<double>("wipe_tower_max_purge_speed")) * 60.f;
}

} // namespace Slic3r::Boss
