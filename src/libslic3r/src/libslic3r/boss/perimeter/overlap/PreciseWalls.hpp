///|/ Copyright (c) preFlight 2025 oozeBot R&D @oozebot
///|/
///|/ preFlight is based on PrusaSlicer and released under AGPLv3 or higher
///|/
#pragma once

#include "libslic3r/Flow.hpp"
#include "libslic3r/libslic3r.h"
#include "Slic3r/Domain/Percentage.hpp"

namespace Slic3r::Boss {

// ================================================================================
// Precise Walls Feature
// ================================================================================
// This feature provides granular control over perimeter-to-perimeter overlap,
// addressing two key issues:
// 1. External perimeter precision - eliminates outer wall deformation by inner walls
// 2. Dimensional accuracy - allows user control over total wall thickness
//
// Unlike OrcaSlicer's boolean approach, we use FloatOrPercentage for maximum flexibility.
// ================================================================================

class PreciseWalls
{
public:
    // Calculate spacing between external perimeter and first internal perimeter
    //
    // Parameters:
    //   ext_flow: Flow for external perimeter
    //   int_flow: Flow for internal perimeters
    //   overlap: User setting (FloatOrPercentage) - can be:
    //            - Percentage: "0%" = no overlap (precise), "21.46%" = standard overlap
    //            - Absolute: "0.05mm" = specific overlap amount
    //
    // Returns: Scaled coordinate for spacing
    static coord_t calculate_external_spacing(const Flow &ext_flow, const Flow &int_flow,
                                               const Domain::FloatOrPercentage &overlap);

    // Calculate spacing between internal perimeters
    //
    // Parameters:
    //   flow: Flow for internal perimeters
    //   overlap: User setting (FloatOrPercentage) - controls overlap between all internal walls
    //
    // Returns: Scaled coordinate for spacing
    static coord_t calculate_perimeter_spacing(const Flow &flow, const Domain::FloatOrPercentage &overlap);

    // Get the standard overlap percentage for optimal bead bonding.
    //
    // Extruded plastic has a stadium-shaped cross-section (rectangle with semicircular ends).
    // The semicircular ends have radius = layer_height / 2. For adjacent beads to bond
    // properly, they must overlap by (1 - π/4) ≈ 21.46% of the layer height.
    //
    // The user-facing percentage is halved so that 100% = complete overlap.
    // Internally we multiply by 2, so:
    //   - User sees 10.73% (optimal) → internally 21.46% of height
    //   - User sees 100% → internally 200% of height = width (for typical 2:1 ratio)
    //
    // Formula: spacing = width - height × (user_percent × 2)
    static constexpr double get_standard_overlap_percent()
    {
        // (1 - π/4) / 2 × 100 = 10.73% (user-facing value, doubled internally)
        return (1.0 - 0.25 * M_PI) * 50.0; // ≈ 10.73%
    }

    // Returns the user's overlap setting if it applies, or the default 10.73% if not.
    // - external_perimeter_overlap only applies with 2+ perimeters
    // - perimeter_perimeter_overlap only applies with 3+ perimeters
    static Domain::FloatOrPercentage get_effective_external_overlap(
        const Domain::FloatOrPercentage &user_overlap, int perimeter_count);

    static Domain::FloatOrPercentage get_effective_perimeter_overlap(
        const Domain::FloatOrPercentage &user_overlap, int perimeter_count);

private:
    // Helper: Apply overlap to calculate spacing from width
    //
    // Parameters:
    //   width: Extrusion width
    //   height: Layer height
    //   overlap: User-specified overlap (FloatOrPercentage)
    //
    // Returns: Spacing value (unscaled)
    static float apply_overlap(float width, float height, const Domain::FloatOrPercentage &overlap);
};

} // namespace Slic3r::Boss
