// src/libslic3r/src/libslic3r/Fill/Fill.hpp
//
// Declares SurfaceFillParams/SurfaceFill/group_fills() so they're directly
// testable, rather than trapped as Fill.cpp implementation details. Moved
// out, not duplicated -- Fill.cpp's own struct definitions and comparison
// operator bodies move here; Fill.cpp keeps only group_fills()'s function
// body and #includes this header.
#pragma once

#include <optional>
#include <vector>

#include "Slic3r/Domain/ConfigDefsFDM.hpp"
#include "libslic3r/ExtrusionRole.hpp"
#include "libslic3r/Flow.hpp"
#include "libslic3r/Surface.hpp"

namespace Slic3r {

class Layer;

struct SurfaceFillParams
{
    // Zero based extruder ID.
    unsigned int extruder = 0;
    // Infill pattern, adjusted for the density etc.
    Domain::InfillPattern pattern = Domain::InfillPattern(0);
    // BOSS fill-dispatch ID (Fill/boss feature registry), or nullopt for
    // a plain native pattern.
    std::optional<int> boss_pattern;

    // in unscaled coordinates
    double spacing = 0.;
    // Angle as provided by the region config, in radians.
    float  angle = 0.f;
    // Is bridging used for this fill? Bridging parameters may be used even if this->flow.bridge() is not set.
    bool   bridge;
    // Non-negative for a bridge.
    float  bridge_angle = 0.f;

    float density = 0.f;
    // Length of the infill anchor along the perimeter line.
    // 1000mm is roughly the maximum length line that fits into a 32bit coord_t.
    float anchor_length     = 1000.f;
    float anchor_length_max = 1000.f;

    // width, height of extrusion, nozzle diameter, is bridge
    Flow flow;

    ExtrusionRole extrusion_role{ExtrusionRole::None};
    // Per-role speed override (bridge/infill/solid/top-solid/over-bridge),
    // read from the region config. Two regions with the same pattern but a
    // different effective speed must stay in separate SurfaceFill groups,
    // or the modifier region's distinct speed is lost when merged.
    float role_speed = 0.f;

    // Index of this entry in a linear vector.
    size_t idx = 0;

    bool operator<(const SurfaceFillParams &rhs) const;
    bool operator==(const SurfaceFillParams &rhs) const;
};

struct SurfaceFill
{
    SurfaceFill(const SurfaceFillParams &params) :
        region_id(size_t(-1)), surface(stCount, ExPolygon()), params(params)
    {}

    size_t          region_id;
    Surface         surface;
    ExPolygons      expolygons;
    SurfaceFillParams params;
};

std::vector<SurfaceFill> group_fills(const Layer &layer);

} // namespace Slic3r
