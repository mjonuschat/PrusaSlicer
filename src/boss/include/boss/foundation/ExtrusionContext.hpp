#pragma once

#include <cstddef>

#include "libslic3r/ExtrusionRole.hpp"

namespace Slic3r {
class PrintRegionConfigView;
}

namespace Slic3r::Boss {

struct ExtrusionContext {
    ExtrusionRole role;
    bool          is_first_layer = false;
    bool          is_object_layer_over_raft = false;
    std::size_t   extruder_id = 0;
    const PrintRegionConfigView *config = nullptr;
};

} // namespace Slic3r::Boss
