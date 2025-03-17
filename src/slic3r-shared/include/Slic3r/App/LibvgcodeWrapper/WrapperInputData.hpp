#pragma once

#include "libslic3r/CustomGCode.hpp"
#include <Slic3r/Biz/libpgcode/Types.hpp>

namespace Slic3r::App::LibvgcodeWrapper {

struct WrapperInputData
{
    Biz::libpgcode::GCodeProducer producer{ Biz::libpgcode::GCodeProducer::Unknown };
    bool sequential_print{ false };
    bool keep_layers_times{ false };
    std::string color_change_gcode;
    std::string pause_print_gcode;
    std::string template_custom_gcode;
    CustomGCode::Info custom_gcode_info;
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
