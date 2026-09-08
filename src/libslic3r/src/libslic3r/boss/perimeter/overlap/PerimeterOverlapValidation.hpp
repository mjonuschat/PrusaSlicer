///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <vector>

namespace Slic3r {
class Print;
}

namespace Slic3r::Biz::Slicing {
struct Warning;
}

namespace Slic3r::Boss {

// Warns when external_perimeter_overlap/perimeter_perimeter_overlap are set
// as an absolute mm value that falls outside the valid range for the
// object's current perimeter extrusion width -- most often because the
// extrusion width changed after the overlap was tuned.
void validate_perimeter_overlap(const Print &print, std::vector<Biz::Slicing::Warning> &warnings);

} // namespace Slic3r::Boss
