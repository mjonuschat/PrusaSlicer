#pragma once

#include "libslic3r/CustomGCode.hpp"
#include <Slic3r/Biz/libpgcode/Types.hpp>

namespace Slic3r::App::LibvgcodeWrapper {

struct WrapperInputData
{
    Biz::libpgcode::GCodeProducer producer{ Biz::libpgcode::GCodeProducer::Unknown };
    bool sequential_print{ false };
    bool one_extruder_printed_model{ true };
    bool keep_layers_times{ false };
    int8_t only_extruder{ -1 };
    Slic3r::CustomGCode::Info custom_gcode_info;
    Biz::libpgcode::PrintSettings print_settings;
};

struct WrapperSLAInputData
{
    struct Layers
    {
        std::vector<float> zs;
        std::vector<float> times;
    };

    Layers layers;
};

} // namespace Slic3r::App::LibvgcodeWrapper
