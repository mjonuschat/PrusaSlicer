///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <string>
#include <array>

#include <imgui.h>

namespace Slic3r::Domain {
class ConfigItem;
struct ConfigValue;
} // namespace Slic3r::Domain

namespace Slic3r::App {

namespace ConfigItemUtils {

std::string config_item_to_string(const Domain::ConfigItem& config_item);
std::string config_item_to_string(const Domain::ConfigItem& config_item, const Domain::ConfigValue& value);
std::string config_item_tooltip(const Domain::ConfigItem& config_item);

inline constexpr std::array<ImColor, 8> colors = {
    ImColor{255, 0, 0, 255}, // RED
    ImColor{0, 255, 0, 255}, // GREEN
    ImColor{0, 0, 255, 255}, // BLUE
    ImColor{255, 255, 0, 255}, // YELLOW
    ImColor{255, 0, 255, 255}, // MAGENTA
    ImColor{0, 255, 255, 255}, // CYAN
    ImColor{127, 127, 127, 255}, // GRAY (0.5 * 255 = 127.5 → 128)
    ImColor{0, 0, 0, 255} // BLACK
};

} // namespace ConfigItemUtils

} // namespace Slic3r::App
