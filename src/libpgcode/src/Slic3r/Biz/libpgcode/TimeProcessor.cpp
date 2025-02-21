///|/ Copyright (c) Prusa Research 2016 - 2023 Oleksandra Iushchenko @YuSanka, Vojtech Bubník @bubnikv, Filip Sykala @Jony01, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matena @lukasmatena, Vojtech Král @vojtechkral
///|/ Copyright (c) 2019 Sijmen Schoon
///|/
///|/ libpgcode library is released under the terms of the AGPLv3 or higher
///|/

#include "TimeProcessor.hpp"
#include "Slic3r/Biz/libpgcode/Utils.hpp"

namespace Slic3r::Biz::libpgcode {
using namespace Domain;

static float get_value(const std::vector<float>& options, size_t id)
{
    return options.empty() ? 0.0f : (id < options.size()) ? options[id] : options.back();
}

void TimeProcessor::update_machine_accelerations(GCodeFlavor flavor)
{
    for (size_t i = 0; i < TIME_MODES_COUNT; ++i) {
        TimeMachine& machine = machines[i];
        float max_acceleration = get_value(machine_limits.max_acceleration_extruding, i);
        machine.max_acceleration = max_acceleration;
        machine.acceleration = (max_acceleration > 0.0f) ? max_acceleration : DEFAULT_ACCELERATION;
        float max_retract_acceleration = get_value(machine_limits.max_acceleration_retracting, i);
        machine.max_retract_acceleration = max_retract_acceleration;
        machine.retract_acceleration = (max_retract_acceleration > 0.0f) ? max_retract_acceleration : DEFAULT_RETRACT_ACCELERATION;

        float max_travel_acceleration = get_value(machine_limits.max_acceleration_travel, i);
        if (!supports_separate_travel_acceleration(flavor) || machine_limits.usage != MachineLimitsUsageType::EmitToGCode)
            // Only clamp travel acceleration when it is accessible in machine limits.
            max_travel_acceleration = 0.0f;
        machine.max_travel_acceleration = max_travel_acceleration;
        machine.travel_acceleration = (max_travel_acceleration > 0.0f) ? max_travel_acceleration : DEFAULT_TRAVEL_ACCELERATION;
    }
}

float TimeProcessor::minimum_feedrate(TimeMode mode, float feedrate) const
{
    return machine_limits.min_extruding_rate.empty() ? feedrate :
        std::max(feedrate, get_value(machine_limits.min_extruding_rate, size_t(mode)));
}

float TimeProcessor::minimum_travel_feedrate(TimeMode mode, float feedrate) const
{
    return machine_limits.min_travel_rate.empty() ? feedrate :
        std::max(feedrate, get_value(machine_limits.min_travel_rate, size_t(mode)));
}

float TimeProcessor::axis_max_feedrate(TimeMode mode, Axis axis) const
{
    switch (axis)
    {
    case X:  { return get_value(machine_limits.max_feedrate_x, size_t(mode)); }
    case Y:  { return get_value(machine_limits.max_feedrate_y, size_t(mode)); }
    case Z:  { return get_value(machine_limits.max_feedrate_z, size_t(mode)); }
    case E:  { return get_value(machine_limits.max_feedrate_e, size_t(mode)); }
    default: { return 0.0f; }
    }
}

float TimeProcessor::acceleration(TimeMode mode) const
{
    size_t id = size_t(mode);
    return (id < machines.size()) ? machines[id].acceleration : DEFAULT_ACCELERATION;
}

float TimeProcessor::travel_acceleration(TimeMode mode) const
{
    size_t id = size_t(mode);
    return (id < machines.size()) ? machines[id].travel_acceleration : DEFAULT_TRAVEL_ACCELERATION;
}

float TimeProcessor::retract_acceleration(TimeMode mode) const
{
    size_t id = size_t(mode);
    return (id < machines.size()) ? machines[id].retract_acceleration : DEFAULT_RETRACT_ACCELERATION;
}

float TimeProcessor::axis_max_acceleration(TimeMode mode, Axis axis) const
{
    switch (axis)
    {
    case X:  { return get_value(machine_limits.max_acceleration_x, size_t(mode)); }
    case Y:  { return get_value(machine_limits.max_acceleration_y, size_t(mode)); }
    case Z:  { return get_value(machine_limits.max_acceleration_z, size_t(mode)); }
    case E:  { return get_value(machine_limits.max_acceleration_e, size_t(mode)); }
    default: { return 0.0f; }
    }
}

float TimeProcessor::axis_max_jerk(TimeMode mode, Axis axis) const
{
    switch (axis)
    {
    case X:  { return get_value(machine_limits.max_jerk_x, size_t(mode)); }
    case Y:  { return get_value(machine_limits.max_jerk_y, size_t(mode)); }
    case Z:  { return get_value(machine_limits.max_jerk_z, size_t(mode)); }
    case E:  { return get_value(machine_limits.max_jerk_e, size_t(mode)); }
    default: { return 0.0f; }
    }
}

float TimeProcessor::filament_load_time(uint8_t extruder_id, bool is_XL_printer) const
{
    if (is_XL_printer)
        return 4.5f; // FIXME
    return (filament_load_times.empty() || extruder_unloaded) ? 0.0f :
        ((size_t(extruder_id) < filament_load_times.size()) ? filament_load_times[extruder_id] : filament_load_times.front());
}

float TimeProcessor::filament_unload_time(uint8_t extruder_id, bool is_XL_printer) const
{
    if (is_XL_printer)
        return 0.0f; // FIXME
    return (filament_unload_times.empty() || extruder_unloaded) ? 0.0f :
        ((size_t(extruder_id) < filament_unload_times.size()) ? filament_unload_times[extruder_id] : filament_unload_times.front());
}

std::vector<std::pair<CustomGCodeType, std::pair<float, float>>> TimeProcessor::custom_gcode_times(TimeMode mode, bool include_remaining) const
{
    std::vector<std::pair<CustomGCodeType, std::pair<float, float>>> ret;
    if (mode < TimeMode::COUNT) {
        const TimeMachine& machine = machines[size_t(mode)];
        float total_time = 0.0f;
        for (const auto& [type, time] : machine.gcode_time.times) {
            float remaining = include_remaining ? float(machine.time) - total_time : 0.0f;
            ret.push_back({ type, { time, remaining } });
            total_time += time;
        }
    }
    return ret;
}

void TimeProcessor::reset()
{
    machine_envelope_processing_enabled = false;
    extruder_unloaded = true;
    for (size_t i = 0; i < TIME_MODES_COUNT; ++i) {
        machines[i].reset();
    }
    machines[size_t(TimeMode::Normal)].enabled = true;
    machines[size_t(TimeMode::Normal)].line_m73_main_mask = "M73 P%s R%s\n";
    machines[size_t(TimeMode::Normal)].line_m73_stop_mask = "M73 C%s\n";
    machines[size_t(TimeMode::Stealth)].line_m73_main_mask = "M73 Q%s S%s\n";
    machines[size_t(TimeMode::Stealth)].line_m73_stop_mask = "M73 D%s\n";
    filament_load_times.clear();
    filament_unload_times.clear();
    machine_limits.reset();
}

} // namespace Slic3r::Biz::libpgcode
