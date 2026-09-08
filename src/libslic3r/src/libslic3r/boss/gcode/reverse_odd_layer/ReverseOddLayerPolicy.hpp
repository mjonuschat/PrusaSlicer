///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

namespace Slic3r::Boss {

// Decides whether a perimeter loop or an infill polyline should be extruded
// in the opposite direction on odd layers, to reduce stress and warping
// (internal perimeters, infill) or to improve steep overhangs (overhang
// perimeters).
class ReverseOddLayerPolicy
{
public:
    // is_internal and is_overhang describe the loop being emitted: is_internal is
    // true for perimeters that are not the external perimeter, and is_overhang is
    // true when the loop touches an overhang path. Each reversal option only
    // applies to the loop kind it names.
    static bool reverse_perimeter(
        bool is_odd_layer,
        bool is_internal,
        bool is_overhang,
        bool internal_perimeters_reverse,
        bool overhangs_reverse
    );

    static bool reverse_infill(bool is_odd_layer, bool infill_reverse);
};

} // namespace Slic3r::Boss
