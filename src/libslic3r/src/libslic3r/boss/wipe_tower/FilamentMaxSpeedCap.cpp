///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "boss/features/filament-max-speed/FilamentMaxSpeedFeature.hpp"

#include "libslic3r/ConfigViews.hpp"

namespace Slic3r::Boss {

std::optional<float> FilamentMaxSpeedFeature::speed_cap(
    const Slic3r::PrintConfigView& config,
    const std::vector<unsigned>& extruder_candidates
)
{
    // A value of 0.0 means "no limit" for this option, so it's excluded from
    // the minimum instead of being substituted with a fallback -- unlike a
    // typical speed option where 0.0 means "use the default".
    const std::vector<double>& speeds = config.get<std::vector<double>>("filament_max_speed");
    double min{0.0};
    for (unsigned tool_index : extruder_candidates) {
        double value{speeds[tool_index]};
        if (value <= 0.0) {
            continue;
        }
        if (min <= 0.0 || value < min) {
            min = value;
        }
    }
    if (min <= 0.0)
        return std::nullopt;
    return static_cast<float>(min) * 60.f;
}

} // namespace Slic3r::Boss
