///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

namespace Slic3r::Boss {

// Computes how strongly a perimeter loop should blend toward
// small_perimeter_speed, based on its length relative to the configured
// small_perimeter_min_length / small_perimeter_max_length thresholds.
class SmallPerimeterSpeedRatio
{
public:
    // Returns a blend factor in [0.0, 1.0]:
    // - 1.0 at or below min_length (fully use small_perimeter_speed)
    // - 0.0 at or above max_length (fully use the role's normal speed)
    // - a linear interpolation between the two thresholds otherwise
    //
    // All lengths are in millimeters. If max_length <= min_length, the ratio
    // is 1.0 at or below min_length and 0.0 otherwise (no gradient to interpolate).
    static double speed_ratio(double perimeter_length_mm, double min_length, double max_length);
};

} // namespace Slic3r::Boss
