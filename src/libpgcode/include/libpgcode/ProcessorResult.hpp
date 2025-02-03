///|/ Copyright (c) Prusa Research 2016 - 2023 Oleksandra Iushchenko @YuSanka, Vojtech Bubník @bubnikv, Filip Sykala @Jony01, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matena @lukasmatena, Vojtech Král @vojtechkral
///|/ Copyright (c) 2019 Sijmen Schoon
///|/
///|/ libpgcode library is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "libpgcode/LineView.hpp"
#include "libpgcode/Types.hpp"

namespace Slic3r::Biz::libpgcode {

struct FilamentGeometry
{
    float diameter{ 0.0f };
    float area_cross_section{ 0.0f };
};

struct ProcessorResult
{
    GCodeProducer producer{ GCodeProducer::Unknown };
    uint8_t extruders_count{ MIN_EXTRUDERS_COUNT };
    bool spiral_vase_enabled{ false };
    float z_offset{ 0.0f };
    float max_print_height{ 0.0f };
    std::vector<float> filament_diameters;
    std::vector<float> filament_densities;
    std::vector<float> filament_costs;
    std::vector<Slic3r::Vec2f> bed_shape;

    LineView gcode;

    std::vector<std::string> extruder_str_colors;
    std::vector<MoveVertex> moves;
    std::vector<Slic3r::CustomGCode::Item> custom_gcode_per_print_z;
    PrintEstimatedStatistics print_statistics;
    PrintSettings print_settings;
    std::optional<ConflictResult> conflict_result;

    uint32_t id() const;
    void set_new_id();

    FilamentGeometry filament_geometry(uint8_t extruder_id) const;
    uint32_t layer_id_at(uint32_t gcode_id) const;

    std::vector<std::string> color_strings_for_color_print() const;

    void reset();

private:
    uint32_t m_id{ 0 };
};

} // namespace Slic3r::Biz::libpgcode
