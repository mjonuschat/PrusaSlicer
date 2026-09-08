#pragma once

#include <cstddef>

namespace Slic3r::Boss {

// Read-only inputs available to every perimeter-policy feature. Deliberately
// narrow -- do not add a raw PerimeterGenerator::Parameters& here; extend this
// struct field by field as a feature actually needs something.
struct PerimeterPolicyContext {
    int      layer_id = 0;
    bool     is_first_layer = false;
    bool     spiral_vase = false;
    double   fill_density = 0.0; // 0-100
    size_t   extruder_id = 0;
};

// Mirrors the spec's OrderingPolicy struct (§7). Each contributing feature
// modifies only its declared fields, in generator-validated priority order.
struct OrderingPolicy {
    bool   contours_external_first = false;
    bool   holes_external_first = false;
    double min_hole_perimeter_length = 0.0;
    int    disabled_first_layers = 0;
    bool   reverse_odd_layers = false; // unused by this registry; kept for
                                        // spec-shape parity, set directly by
                                        // Task 5's own hook, not folded here.
};

} // namespace Slic3r::Boss
