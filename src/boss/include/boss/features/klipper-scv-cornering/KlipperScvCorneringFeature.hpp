#pragma once
#include <string_view>
namespace Slic3r::Boss {
// This feature joins no capability registry -- it has no config surface and
// no registry hook. It exists only so the manifest has a syntactically valid
// trait/header pair; the algorithm lives in KlipperCorneringModel, called
// directly from TimeProcessor::update_machine_accelerations and
// ProcessorImpl::process_gcode_line/process_G1.
struct KlipperScvCorneringFeature {
    static constexpr int id = 10502;
    static constexpr std::string_view key = "klipper-scv-cornering";
    static constexpr std::string_view label = "Klipper square corner velocity cornering model";
};
} // namespace Slic3r::Boss
