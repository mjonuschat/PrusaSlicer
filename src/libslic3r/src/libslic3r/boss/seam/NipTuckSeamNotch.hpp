///|/ Copyright (c) 2024 - 2026 Morton Jonuschat @mjonuschat
///|/ Copyright (c) preFlight 2026 oozeBot R&D @oozebot
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
// src/libslic3r/src/libslic3r/boss/seam/NipTuckSeamNotch.hpp
#pragma once

#include "libslic3r/GCode/ExtrusionOrder.hpp"

namespace Slic3r::Domain {
class ConfigView;
}

namespace Slic3r::Boss {

// Apply a V-notch to a single (external, inner) perimeter pair. inner_perim
// may be nullptr when no inner perimeter should be trimmed (e.g. the far
// external in a shared-inner split, per apply_seam_notch's own handling).
void apply_seam_notch_pair(
    GCode::ExtrusionOrder::Perimeter &ext_perim,
    GCode::ExtrusionOrder::Perimeter *inner_perim,
    const Domain::ConfigView &config,
    int layer_index);

// Pair every external perimeter in `perimeters` with its closest inner
// perimeter by seam proximity and apply the configured notch to each pair,
// appending any newly-split inner-perimeter halves to `perimeters`.
void apply_seam_notch(
    std::vector<GCode::ExtrusionOrder::Perimeter> &perimeters,
    const Domain::ConfigView &config,
    int layer_index);

} // namespace Slic3r::Boss
