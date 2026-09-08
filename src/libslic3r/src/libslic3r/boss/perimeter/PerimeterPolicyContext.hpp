#pragma once

#include <cstddef>

namespace Slic3r {
class PrintRegionConfigView;
namespace PerimeterGenerator { struct Parameters; }
} // namespace Slic3r

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

    // Non-owning; the caller's PerimeterGenerator::Parameters::config outlives
    // every registry call made against this context.
    const PrintRegionConfigView *config = nullptr;
};

// Populates every field of PerimeterPolicyContext from a perimeter
// generator's parameters, so both the Arachne and Classic call sites build
// an identical context instead of each hand-populating a different subset
// of fields.
PerimeterPolicyContext make_perimeter_policy_context(const PerimeterGenerator::Parameters &params, size_t extruder_id);

// Mirrors the spec's OrderingPolicy struct (§7). Each contributing feature
// modifies only its declared fields, in generator-validated priority order.
struct OrderingPolicy {
    bool   contours_external_first = false;
    bool   holes_external_first = false;
    double min_hole_perimeter_length = 0.0;
    int    disabled_first_layers = 0;
    // Not populated by any current perimeter-policy feature; kept for
    // OrderingPolicy's spec-shape parity.
    bool   reverse_odd_layers = false;
};

} // namespace Slic3r::Boss
