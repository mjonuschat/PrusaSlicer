///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "libslic3r/boss/gcode/reverse_odd_layer/ReverseOddLayerPolicy.hpp"

namespace Slic3r::Boss {

bool ReverseOddLayerPolicy::reverse_perimeter(
    bool is_odd_layer,
    bool is_internal,
    bool is_overhang,
    bool internal_perimeters_reverse,
    bool overhangs_reverse
)
{
    const bool should_reverse_internal = is_internal && internal_perimeters_reverse;
    const bool should_reverse_overhang = is_overhang && overhangs_reverse;
    return is_odd_layer && (should_reverse_internal || should_reverse_overhang);
}

bool ReverseOddLayerPolicy::reverse_infill(bool is_odd_layer, bool infill_reverse)
{
    return is_odd_layer && infill_reverse;
}

} // namespace Slic3r::Boss
