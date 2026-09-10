#pragma once

#include <cstddef>

#include "libslic3r/ExtrusionRole.hpp"

namespace Slic3r {
class PrintRegionConfigView;
}

namespace Slic3r::Boss {

struct BossExtrudeConfigOverrides;

struct ExtrusionContext {
    ExtrusionRole role;
    bool          is_first_layer = false;
    bool          is_object_layer_over_raft = false;
    std::size_t   extruder_id = 0;
    const PrintRegionConfigView *config = nullptr;

    // generate_travel_gcode() (GCode.cpp) never carries a raw ConfigView -- only the
    // pre-resolved Biz::Slicing::ExtrudeConfig -- so its two travel roles are flagged
    // here instead of routed through the extrusion-role priority cascade.
    bool is_travel = false;
    bool is_short_distance_travel = false;

    // _extrude()/extrude_smooth_path() (GCode.cpp) likewise only carry
    // Biz::Slicing::ExtrudeConfig, not a raw ConfigView, so per-role values reach
    // here via ExtrudeConfig::boss instead of ctx.config.
    const BossExtrudeConfigOverrides *boss_config = nullptr;
};

} // namespace Slic3r::Boss
