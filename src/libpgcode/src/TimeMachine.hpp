///|/ Copyright (c) Prusa Research 2016 - 2023 Oleksandra Iushchenko @YuSanka, Vojtech Bubník @bubnikv, Filip Sykala @Jony01, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matena @lukasmatena, Vojtech Král @vojtechkral
///|/ Copyright (c) 2019 Sijmen Schoon
///|/
///|/ libpgcode library is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "libpgcode/PostProcessorConfig.hpp"
#include "TimeBlock.hpp"

namespace Slic3r::Biz::libpgcode {

struct ProcessorResult;

struct ActualSpeedMove
{
    uint32_t move_id{ 0 };
    float actual_feedrate{ 0.0f };
    std::optional<Slic3r::Vec3f> position;
    std::optional<float> delta_extruder;
    std::optional<float> feedrate;
    std::optional<float> width;
    std::optional<float> height;
    std::optional<float> mm3_per_mm;
    std::optional<float> fan_speed;
    std::optional<float> temperature;
};

struct CustomGCodeTime
{
    bool needed{ false };
    float cache{ 0.0f };
    std::vector<std::pair<Slic3r::CustomGCode::Type, float>> times;

    void reset();
};

struct TimeMachineState
{
    float feedrate{ 0.0f }; // mm/s
    float safe_feedrate{ 0.0f }; // mm/s
    Slic3r::Vec4f axis_feedrate{ Slic3r::Vec4f::Zero() }; // mm/s
    Slic3r::Vec4f abs_axis_feedrate{ Slic3r::Vec4f::Zero() }; // mm/s

    void reset();
};

struct TimeMachine
{
    bool enabled{ false };
    float extrude_factor_override_percentage{ 1.0f };
    float acceleration{ 0.0f }; // mm/s^2
    // hard limit for the acceleration, to which the firmware will clamp.
    float max_acceleration{ 0.0f }; // mm/s^2
    float retract_acceleration{ 0.0f }; // mm/s^2
    // hard limit for the acceleration, to which the firmware will clamp.
    float max_retract_acceleration{ 0.0f }; // mm/s^2
    float travel_acceleration{ 0.0f }; // mm/s^2
    // hard limit for the travel acceleration, to which the firmware will clamp.
    float max_travel_acceleration{ 0.0f }; // mm/s^2
    float first_layer_time{ 0.0f };
    // We accumulate total print time in doubles to reduce the loss of precision
    // while adding big floating numbers with small float numbers.
    double time{ 0.0 }; // s
    std::string line_m73_main_mask;
    std::string line_m73_stop_mask;
    std::vector<TimeBlock> blocks;
    std::vector<ActualSpeedMove> actual_speed_moves;
    std::vector<G1LinesCacheItem> g1_times_cache;
    std::vector<StopTime> stop_times;
    CustomGCodeTime gcode_time;
    TimeMachineState curr;
    TimeMachineState prev;

    void calculate_time(ProcessorResult& result, TimeMode mode, size_t keep_last_n_blocks = 0, float additional_time = 0.0f);
    void set_acceleration(float value);
    void set_travel_acceleration(float value);
    void set_retract_acceleration(float value);
    void reset();
};

// Calculates the maximum allowable speed at this point when you must be able to reach target_velocity using the 
// acceleration within the allotted distance.
float max_allowable_speed(float acceleration, float target_velocity, float distance);

} // namespace Slic3r::Biz::libpgcode
