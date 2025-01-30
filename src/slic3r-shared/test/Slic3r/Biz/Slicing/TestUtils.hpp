#pragma once

#include "libslic3r/Model.hpp"
#include <chrono>

namespace Slic3r::Tests {
void precise_sleep(const std::chrono::milliseconds duration);

Slic3r::Model generate_cubes(const int count, const int row_size);

double get_cubes_filament_used(const Slic3r::Model &model);

Slic3r::DynamicPrintConfig get_config();

}
