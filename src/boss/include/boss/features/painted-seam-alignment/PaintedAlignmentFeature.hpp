#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "libslic3r/GCode/SeamAligned.hpp"
#include "libslic3r/GCode/SeamChoice.hpp"
#include "libslic3r/GCode/SeamShells.hpp"
#include "libslic3r/Point.hpp"

namespace Slic3r::Boss {

struct PaintedAlignmentFeature {
    static constexpr int id = 10507;
    static constexpr std::string_view key = "painted-seam-alignment";
    static constexpr std::string_view label = "Painted seam alignment/blending";

    // Exposed for direct unit testing; also used internally by
    // get_seam_candidate_override.
    static std::vector<Vec2d> cluster_positions(
        const std::vector<Vec2d> &positions, double cluster_radius);
    static std::optional<Vec2d> get_enforcer_centroid_near(
        const Seams::Perimeters::Perimeter &perimeter, const Vec2d &reference_position,
        double max_distance);

    static std::optional<std::vector<Vec2d>> get_starting_positions_override(
        const Seams::Shells::Shell<> &shell, const Seams::Aligned::Params &params);
    static std::optional<Seams::Aligned::SeamCandidate> get_seam_candidate_override(
        const Seams::Shells::Shell<> &shell,
        const Vec2d &starting_position,
        const Seams::Aligned::SeamChoiceVisibility &visibility_calculator,
        const Seams::Aligned::Params &params,
        const std::vector<std::vector<double>> &precalculated_visibility,
        const std::vector<Seams::Aligned::LeastVisiblePoint> &least_visible_points);
};

} // namespace Slic3r::Boss
