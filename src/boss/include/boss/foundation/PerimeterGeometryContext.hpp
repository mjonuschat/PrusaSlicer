// src/boss/include/boss/foundation/PerimeterGeometryContext.hpp
#pragma once

#include <vector>

namespace Slic3r::GCode::ExtrusionOrder {
struct Perimeter;
}

namespace Slic3r::Domain {
class ConfigView;
}

namespace Slic3r::Boss {

// Mutable view over one island's already-seam-placed perimeters, available
// to registry features right after extract_perimeter_extrusions() produces
// them and before they're consumed by GCodeGenerator::extrude_perimeters()
// (which takes them by const reference -- this is the last point at which
// a feature can still mutate path geometry, insert a split perimeter, or
// otherwise transform the vector). Non-owning -- do not store a
// PerimeterGeometryContext past the call that constructs it.
struct PerimeterGeometryContext {
    std::vector<GCode::ExtrusionOrder::Perimeter> &perimeters;
    const Domain::ConfigView                      *config = nullptr;
    int                                             layer_index = 0;
};

} // namespace Slic3r::Boss
