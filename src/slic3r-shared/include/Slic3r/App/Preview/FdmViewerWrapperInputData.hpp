#pragma once

#include <Slic3r/Biz/libpgcode/Types.hpp>

namespace Slic3r::App::Preview {

struct FdmViewerWrapperInputData
{
    Biz::libpgcode::GCodeProducer producer{ Biz::libpgcode::GCodeProducer::Unknown };
    bool sequential_print{ false };
    bool keep_layers_times{ false };
    std::string color_change_gcode;
    std::string pause_print_gcode;
    std::string template_custom_gcode;
    Domain::CustomGCode::Info custom_gcode_info;
    Biz::libpgcode::PrintSettings print_settings;
};

} // namespace Slic3r::App::Preview
