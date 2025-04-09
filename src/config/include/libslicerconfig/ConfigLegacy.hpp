#pragma once
#include <string>

#include "libslicerconfig/Config.hpp"

// Loads config from INI / GCODE / BGCODE produced by PrusaSlicer < 3.0.0 and converts
// all matching keys into the provided ConfigBox.
void load_from_legacy_file(const std::string& filename, ConfigBox& config);