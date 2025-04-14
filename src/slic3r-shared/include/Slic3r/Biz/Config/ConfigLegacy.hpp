#pragma once
#include <string>

#include "Slic3r/Domain/Config.hpp"

namespace Slic3r::Biz {

// Loads config from INI / GCODE / BGCODE produced by PrusaSlicer < 3.0.0 and converts
// all matching keys into the provided ConfigBox.
void load_from_legacy_file(const std::string& filename, Domain::ConfigBox& config);

} // namespace Slic3r::Biz
