#pragma once
#include <string>
#include <string_view>
namespace Slic3r::Boss {
struct MergeVelocityLimitFeature {
    static constexpr int id = 10503;
    static constexpr std::string_view key = "merge-velocity-limit";
    static constexpr std::string_view label = "Merge consecutive Klipper velocity-limit commands";
    static std::string filter_layer(std::string gcode);
};
} // namespace Slic3r::Boss
