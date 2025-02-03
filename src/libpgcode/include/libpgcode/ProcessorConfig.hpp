///|/ Copyright (c) Prusa Research 2016 - 2023 Oleksandra Iushchenko @YuSanka, Vojtech Bubník @bubnikv, Filip Sykala @Jony01, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matena @lukasmatena, Vojtech Král @vojtechkral
///|/ Copyright (c) 2019 Sijmen Schoon
///|/
///|/ libpgcode library is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "libpgcode/Types.hpp"

namespace Slic3r::Biz::libpgcode {

enum class MachineLimitsUsageType : uint8_t
{
    EmitToGCode,
    TimeEstimateOnly,
    Ignore,
    COUNT
};

static constexpr size_t MACHINE_LIMITS_USAGE_TYPES_COUNT = size_t(MachineLimitsUsageType::COUNT);

struct MachineLimitsConfig
{
    MachineLimitsUsageType usage{ MachineLimitsUsageType::TimeEstimateOnly };

    std::vector<float> max_acceleration_x;
    std::vector<float> max_acceleration_y;
    std::vector<float> max_acceleration_z;
    std::vector<float> max_acceleration_e;
    std::vector<float> max_feedrate_x;
    std::vector<float> max_feedrate_y;
    std::vector<float> max_feedrate_z;
    std::vector<float> max_feedrate_e;
    std::vector<float> max_jerk_x;
    std::vector<float> max_jerk_y;
    std::vector<float> max_jerk_z;
    std::vector<float> max_jerk_e;

    std::vector<float> max_acceleration_extruding;
    std::vector<float> max_acceleration_retracting;
    std::vector<float> max_acceleration_travel;

    std::vector<float> min_travel_rate;
    std::vector<float> min_extruding_rate;

    //
    // set to default all missing values
    //
    std::vector<std::string> validate();
    void reset();
};

struct FilamentsConfig
{
    std::vector<float> diameters;
    std::vector<float> densities;
    std::vector<float> costs;
    std::vector<float> load_times;
    std::vector<float> unload_times;

    void reset();
};

struct ExtrudersConfig
{
    uint8_t count{ MIN_EXTRUDERS_COUNT };
    std::vector<Slic3r::Vec3f> offsets;
    std::vector<std::string> str_colors;
    std::vector<int> temps_config;
    std::vector<int> temps_first_layer_config;

    void reset();
};

typedef std::function<void(const std::string&)> ProcessorLogCallback;
typedef std::function<double(const std::string_view, size_t*)> StringToDoubleDecimalPointCallback;
typedef std::function<std::string(double, int)> FloatToStringDecimalPointCallback;

struct ProcessorCallbacksConfig
{
    ProcessorLogCallback cb_log;
    StringToDoubleDecimalPointCallback cb_string_to_double_decimal_point;
    FloatToStringDecimalPointCallback cb_float_to_string_decimal_point;
};

struct ProcessorConfig
{
    GCodeProducer producer{ GCodeProducer::Unknown };
    Slic3r::GCodeFlavor flavor{ Slic3r::gcfRepRapSprinter };
    bool use_volumetric_e{ false };
    bool export_remaining_time_enabled{ false };
    bool stealth_time_estimator_enabled{ false };
    bool spiral_vase_enabled{ false };
    bool is_XL_printer{ false };
    bool single_extruder_multi_material{ false };
    float z_offset{ 0.0f };
    float max_print_height{ 0.0f };
    float first_layer_height{ 0.0f };
    float parking_pos_retraction{ 0.0f };
    float extra_loading_move{ 0.0f };
    float kisslicer_toolchange_time_correction{ 0.0f };
    std::vector<Slic3r::Vec2f> bed_shape;
    FilamentsConfig filaments;
    ExtrudersConfig extruders;
    MachineLimitsConfig machine_limits;
    PrintSettings print_settings;
    ProcessorCallbacksConfig callbacks;

    void reset();
};

ProcessorConfig extract_processor_config_from_prusaslicer_gcode(const std::string& gcode, StringToDoubleDecimalPointCallback cb);

// updated to AnkerMake Studio 1.5.24
ProcessorConfig extract_processor_config_from_ankermakestudio_gcode(const std::string& gcode, StringToDoubleDecimalPointCallback cb);
// updated to BambuStudio 1.9.7.52
ProcessorConfig extract_processor_config_from_bambustudio_gcode(const std::string& gcode, StringToDoubleDecimalPointCallback cb);
// updated to CraftWare 1.2.1.707
ProcessorConfig extract_processor_config_from_craftware_gcode(const std::string& gcode, StringToDoubleDecimalPointCallback cb);
// updated to Cura 5.8.1
ProcessorConfig extract_processor_config_from_cura_gcode(const std::string& gcode, StringToDoubleDecimalPointCallback cb);
// updated to KISSlicer 23.05
ProcessorConfig extract_processor_config_from_kisslicer_gcode(const std::string& gcode, StringToDoubleDecimalPointCallback cb);
// updated to ideaMaker 5.1.2
ProcessorConfig extract_processor_config_from_ideamaker_gcode(const std::string& gcode, StringToDoubleDecimalPointCallback cb);
// updated to Orcaslicer 2.1.1
ProcessorConfig extract_processor_config_from_orcaslicer_gcode(const std::string& gcode, StringToDoubleDecimalPointCallback cb);
// updated to Simplify3D 5.1.2
ProcessorConfig extract_processor_config_from_simplify3d_gcode(const std::string& gcode, StringToDoubleDecimalPointCallback cb);
// updated to SuperSlicer 2.5.59.13
ProcessorConfig extract_processor_config_from_superslicer_gcode(const std::string& gcode, StringToDoubleDecimalPointCallback cb);
// updated to XDesktop 3.0.2
ProcessorConfig extract_processor_config_from_xdesktop_gcode(const std::string& gcode, StringToDoubleDecimalPointCallback cb);

} // namespace Slic3r::Biz::libpgcode
