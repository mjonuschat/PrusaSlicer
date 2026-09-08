///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "libslic3r/boss/perimeter/small_threshold/SmallPerimeterSpeedRatio.hpp"

#include <algorithm>

namespace Slic3r::Boss {

double SmallPerimeterSpeedRatio::speed_ratio(double perimeter_length_mm, double min_length, double max_length)
{
    if (perimeter_length_mm <= min_length) {
        return 1.0;
    }
    if (perimeter_length_mm >= max_length || max_length <= min_length) {
        return 0.0;
    }

    const double factor = (perimeter_length_mm - min_length) / (max_length - min_length);
    return std::clamp(1.0 - factor, 0.0, 1.0);
}

} // namespace Slic3r::Boss
