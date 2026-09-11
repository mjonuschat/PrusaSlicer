#pragma once

#include <cstddef>

#include "libslic3r/ExtrusionRole.hpp"

namespace Slic3r {
class PrintRegionConfigView;
class Layer;
}

namespace Slic3r::Biz::Slicing {
struct ExtrudeConfig;
}

namespace Slic3r::Boss {

struct ExtrusionContext {
    ExtrusionRole role;
    bool          is_first_layer = false;
    bool          is_object_layer_over_raft = false;
    std::size_t   extruder_id = 0;
    const PrintRegionConfigView *config = nullptr;

    double        path_length = 0.0;
    const Layer  *layer = nullptr;

    // Only meaningful when layer != nullptr.
    double query_point_x = 0.0;
    double query_point_y = 0.0;

    // generate_travel_gcode() (GCode.cpp) never carries a raw ConfigView -- only the
    // pre-resolved Biz::Slicing::ExtrudeConfig -- so its two travel roles are flagged
    // here instead of routed through the extrusion-role priority cascade.
    bool is_travel = false;
    bool is_short_distance_travel = false;

    // _extrude()/extrude_smooth_path()/generate_travel_gcode() (GCode.cpp) never
    // carry a raw ConfigView -- only the pre-resolved Biz::Slicing::ExtrudeConfig --
    // so per-role values reach a feature via this pointer's ::boss member instead
    // of ctx.config. Exposing the full ExtrudeConfig (not just its ::boss member)
    // also lets a feature's own per-role selection ride on a foundation-owned
    // per-role value it doesn't itself store, such as an acceleration field.
    const Biz::Slicing::ExtrudeConfig *extrude_config = nullptr;
};

} // namespace Slic3r::Boss
