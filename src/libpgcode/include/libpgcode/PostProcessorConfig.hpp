///|/ Copyright (c) Prusa Research 2016 - 2023 Oleksandra Iushchenko @YuSanka, Vojtech Bubník @bubnikv, Filip Sykala @Jony01, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matena @lukasmatena, Vojtech Král @vojtechkral
///|/ Copyright (c) 2019 Sijmen Schoon
///|/
///|/ libpgcode library is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "libpgcode/Types.hpp"

namespace Slic3r::Biz::libpgcode {

struct G1LinesCacheItem
{
    uint32_t id{ 0 };
    uint32_t remaining_internal_g1_lines{ 0 };
    float elapsed_time{ 0.0f };
};

struct StopTime
{
    uint32_t g1_line_id{ 0 };
    float elapsed_time{ 0.0f };
};

struct TimeMachineData
{
    bool enabled{ false };
    float time{ 0.0f };
    float first_layer_time{ 0.0f };
    std::string line_m73_main_mask;
    std::string line_m73_stop_mask;
    std::vector<G1LinesCacheItem> g1_times_cache;
    std::vector<StopTime> stop_times;

    void reset();
};

struct PostProcessorConfig
{
    bool export_remaining_time_enabled{ false };
    bool backtrace_enabled{ false };
    bool is_XL_printer{ false };
    std::array<TimeMachineData, TIME_MODES_COUNT> time_machines;
    std::vector<int> extruder_temps_config;
    std::vector<int> extruder_temps_first_layer_config;

    void reset();
};

} // namespace Slic3r::Biz::libpgcode
