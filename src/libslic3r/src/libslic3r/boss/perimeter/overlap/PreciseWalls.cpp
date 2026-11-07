///|/ Copyright (c) preFlight 2025 oozeBot R&D @oozebot
///|/
///|/ preFlight is based on PrusaSlicer and released under AGPLv3 or higher
///|/
#include "libslic3r/boss/perimeter/overlap/PreciseWalls.hpp"

#include <algorithm>
#include <cmath>

namespace Slic3r::Boss {

// ================================================================================
// Helper Functions
// ================================================================================

float PreciseWalls::apply_overlap(float width, float height, const Domain::FloatOrPercentage &overlap)
{
    // Calculate the overlap amount
    float overlap_amount;

    if (overlap.is_percentage())
    {
        // Percentage mode: Overlap is calculated from both layer height and extrusion width.
        //
        // The geometric constant (1 - π/4) ≈ 21.46% of layer height is needed for optimal
        // bead bonding due to the semicircular cross-section of extruded plastic.
        //
        // We scale the user's percentage so that:
        //   - 10.73% (default) = optimal bonding (internally 21.46% of height)
        //   - 100% = complete overlap (spacing = 0) for typical width = 2×height
        //
        // Formula: overlap_amount = height × (user_percent × 2 / 100)
        // This means 100% user input → 200% of height → overlap = width (when width = 2h)
        float clamped_percent = std::min(float(overlap.percentage().value), 100.0f);
        overlap_amount = height * (clamped_percent * 2.0f / 100.0f);
    }
    else
    {
        // Absolute mode: Use the specified mm value directly
        overlap_amount = std::min(float(overlap.float_value()), width);
    }

    // Spacing = width - overlap
    float spacing = width - overlap_amount;

    // Very small spacing can cause issues in skeletal trapezoidation.
    // Use minimum 20% of width (max 80% overlap) to ensure stability.
    // The UI also limits perimeter/perimeter overlap to 80% max.
    float min_spacing = width * 0.20f; // 20% of width minimum (max 80% overlap)
    if (spacing < min_spacing)
    {
        spacing = min_spacing;
    }

    return spacing;
}

// ================================================================================
// Public API
// ================================================================================

coord_t PreciseWalls::calculate_external_spacing(const Flow &ext_flow, const Flow &int_flow,
                                                  const Domain::FloatOrPercentage &overlap)
{
    // Calculate individual spacings with the specified overlap
    float ext_spacing = apply_overlap(ext_flow.width(), ext_flow.height(), overlap);
    float int_spacing = apply_overlap(int_flow.width(), int_flow.height(), overlap);

    // Average the two spacings (matches ext_perimeter_spacing2 calculation)
    float avg_spacing = 0.5f * (ext_spacing + int_spacing);

    return coord_t(scale_(avg_spacing));
}

coord_t PreciseWalls::calculate_perimeter_spacing(const Flow &flow, const Domain::FloatOrPercentage &overlap)
{
    float spacing = apply_overlap(flow.width(), flow.height(), overlap);
    return coord_t(scale_(spacing));
}

Domain::FloatOrPercentage PreciseWalls::get_effective_external_overlap(
    const Domain::FloatOrPercentage &user_overlap, int perimeter_count)
{
    // External perimeter overlap only matters with 2+ perimeters
    // (need external + at least one internal perimeter)
    if (perimeter_count < 2)
    {
        // Return default overlap - user setting doesn't apply
        return Domain::FloatOrPercentage(Domain::Percentage{get_standard_overlap_percent()});
    }
    return user_overlap;
}

Domain::FloatOrPercentage PreciseWalls::get_effective_perimeter_overlap(
    const Domain::FloatOrPercentage &user_overlap, int perimeter_count)
{
    // Perimeter/perimeter overlap only matters with 3+ perimeters
    // (need at least two internal perimeters adjacent to each other)
    if (perimeter_count < 3)
    {
        // Return default overlap - user setting doesn't apply
        return Domain::FloatOrPercentage(Domain::Percentage{get_standard_overlap_percent()});
    }
    // Higher values cause crashes in skeletal trapezoidation algorithm
    if (user_overlap.is_percentage() && user_overlap.percentage().value > 80.0)
    {
        return Domain::FloatOrPercentage(Domain::Percentage{80.0});
    }
    return user_overlap;
}

} // namespace Slic3r::Boss
