#pragma once

#include "libslic3r/ConfigViews.hpp"

namespace Slic3r {

std::string get_extrusion_axis(const PrintConfigView &cfg);

bool is_XL_printer(const PrintConfigView &cfg);

}

