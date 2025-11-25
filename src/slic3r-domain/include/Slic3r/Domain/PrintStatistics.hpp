#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <optional>
#include <map>
#include "Slic3r/Domain/CustomGCode.hpp"
#include "Slic3r/Domain/GCodeExtrusionRole.hpp"

namespace Slic3r::Domain {

struct BasicPrintStatistics
{
    struct TimeStatistics
    {
        float first_layer_time{};
        float time{};
        std::vector<std::pair<Domain::CustomGCode::Type, std::pair<float, float>>>
            custom_gcode_times;
    };

    TimeStatistics normal_mode_time;
    std::optional<TimeStatistics> silent_mode_time;
    std::vector<float> volumes_per_color_change;
    std::map<uint8_t, float> volumes_per_extruder;
    std::map<uint8_t, float> cost_per_extruder;
    std::map<Domain::GCodeExtrusionRole, std::pair<float, float>> used_filaments_per_role;
};

struct ExtraPrintStatistics
{
    int total_toolchanges{};
    double total_wipe_tower_cost{};
    double total_wipe_tower_filament{};
    double total_wipe_tower_filament_weight{};
    std::vector<unsigned int> printing_extruders;
    unsigned int initial_extruder_id{};
    std::string initial_filament_type;
    std::vector<std::string> printing_filament_types;
};

} // namespace Slic3r::Domain
