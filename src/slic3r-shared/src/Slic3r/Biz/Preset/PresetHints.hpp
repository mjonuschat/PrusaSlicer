///|/ Copyright (c) Prusa Research 2017 - 2020 Oleksandra Iushchenko @YuSanka, Vojtěch Bubník @bubnikv
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <string>

#include "libslic3r/Preset.hpp"

namespace Slic3r::Biz::Preset {
    
struct PresetInteractorConfigContainerContext;

// GUI utility functions to produce hint messages from the current profile.
class PresetHints
{
public:
    // Produce a textual description of the cooling logic of a currently active filament.
    static std::string cooling_description(const Slic3r::Preset &preset);
    
    // Produce a textual description of the maximum flow achived for the current configuration
    // (the current printer, filament and print settigns).
    // This description will be useful for getting a gut feeling for the maximum volumetric
    // print speed achievable with the extruder.
    static std::string maximum_volumetric_flow_description(const Preset::PresetInteractorConfigContainerContext& ccc, int idx_extruder = 0);

    // Produce a textual description of a recommended thin wall thickness
    // from the provided number of perimeters and the external / internal perimeter width.
    static std::string recommended_thin_wall_thickness(const Preset::PresetInteractorConfigContainerContext& ccc);

    // Produce a textual explanation of the combined effects of the top/bottom_solid_layers
    // versus top/bottom_min_shell_thickness. Which of the two values wins depends
    // on the active layer height.
    static std::string top_bottom_shell_thickness_explanation(const Preset::PresetInteractorConfigContainerContext& ccc);
};

}
