///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <string>

#include <imgui.h>

namespace Slic3r::App {

/**
 * This is a temporary class, we will probably grab data directly somewhere from
 * PresetBundle/PresetConfig
 */
struct MaterialState
{
    MaterialState() {}

    MaterialState(ImColor color, const std::string& name, float nozzle)
        : color(color), name(name), nozzle(nozzle)
    {}

    ImColor color = IM_COL32_WHITE;
    std::string name;
    float nozzle = 0;
};

} // namespace Slic3r::App
