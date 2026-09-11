#include "boss/features/painted-seam-alignment/PaintedAlignmentFeature.hpp"

#include <algorithm>
#include <cstddef>

#include "libslic3r/GCode/SeamChoice.hpp"
#include "libslic3r/GCode/SeamPerimeters.hpp"

namespace Slic3r::Boss {

using Seams::Perimeters::AngleType;
using Seams::Perimeters::Perimeter;
using Seams::Perimeters::PointType;
using Seams::SeamChoice;
using Seams::Aligned::LeastVisiblePoint;
using Seams::Aligned::Params;
using Seams::Aligned::SeamCandidate;
using Seams::Aligned::SeamChoiceVisibility;
using Seams::Shells::Shell;

std::optional<Vec2d> PaintedAlignmentFeature::get_enforcer_centroid_near(
    const Perimeter &perimeter, const Vec2d &reference_position, const double max_distance)
{
    Vec2d sum = Vec2d::Zero();
    int count = 0;
    for (std::size_t i = 0; i < perimeter.positions.size(); ++i) {
        if (perimeter.point_types[i] == PointType::enforcer &&
            (perimeter.positions[i] - reference_position).norm() <= max_distance) {
            sum += perimeter.positions[i];
            ++count;
        }
    }
    if (count == 0)
        return std::nullopt;
    return sum / static_cast<double>(count);
}

std::vector<Vec2d> PaintedAlignmentFeature::cluster_positions(
    const std::vector<Vec2d> &positions, const double cluster_radius)
{
    if (positions.empty())
        return {};

    std::vector<bool> assigned(positions.size(), false);
    std::vector<Vec2d> centroids;

    for (std::size_t i = 0; i < positions.size(); ++i) {
        if (assigned[i])
            continue;
        Vec2d sum = positions[i];
        int count = 1;
        assigned[i] = true;
        for (std::size_t j = i + 1; j < positions.size(); ++j) {
            if (assigned[j])
                continue;
            if ((positions[j] - positions[i]).norm() <= cluster_radius) {
                sum += positions[j];
                ++count;
                assigned[j] = true;
            }
        }
        centroids.push_back(sum / static_cast<double>(count));
    }
    return centroids;
}

std::optional<std::vector<Vec2d>> PaintedAlignmentFeature::get_starting_positions_override(
    const Shell<> &shell, const Params &params)
{
    const Perimeter &perimeter{shell.front().boundary};
    std::vector<Vec2d> enforcers{Seams::Perimeters::extract_points(perimeter, PointType::enforcer)};
    if (enforcers.empty())
        return std::nullopt;
    return cluster_positions(enforcers, params.max_detour);
}

std::optional<SeamCandidate> PaintedAlignmentFeature::get_seam_candidate_override(
    const Shell<> &shell,
    const Vec2d &starting_position,
    const SeamChoiceVisibility &visibility_calculator,
    const Params &params,
    const std::vector<std::vector<double>> &precalculated_visibility,
    const std::vector<LeastVisiblePoint> &least_visible_points)
{
    if (Seams::Perimeters::extract_points(shell.front().boundary, PointType::enforcer).empty())
        return std::nullopt;

    std::vector<double> choice_visibilities(shell.size(), 1.0);

    std::vector<SeamChoice> choices{Seams::Aligned::get_shell_seam(
        shell,
        [&, reference_position{starting_position}, prev_z{0.0}](
            const Perimeter &perimeter, std::size_t slice_index) mutable {
            // The 0.25 base factor was tuned at 0.1mm reference height.
            constexpr double base_blend = 0.25;
            constexpr double reference_layer_height = 0.1;
            const double current_z = perimeter.slice_z;
            const double layer_height =
                (prev_z > 0.0) ? (current_z - prev_z) : reference_layer_height;
            prev_z = current_z;
            const double blend_factor =
                std::clamp(base_blend * (layer_height / reference_layer_height), base_blend, 0.9);

            Vec2d search_target = reference_position;
            bool has_nearby_enforcers = false;
            if (auto centroid =
                    get_enforcer_centroid_near(perimeter, reference_position, params.max_detour)) {
                search_target = *centroid;
                has_nearby_enforcers = true;
            }

            SeamChoice candidate{Seams::choose_seam_point(
                perimeter, Seams::Aligned::Impl::Nearest{search_target, params.max_detour})};
            const bool is_too_far{
                (candidate.position - reference_position).norm() > params.max_detour};
            const LeastVisiblePoint &least_visible{least_visible_points[slice_index]};

            const bool is_on_edge{
                candidate.previous_index == candidate.next_index &&
                perimeter.angle_types[candidate.next_index] != AngleType::smooth};

            if (is_on_edge)
                choice_visibilities[slice_index] =
                    precalculated_visibility[slice_index][candidate.previous_index];
            else
                choice_visibilities[slice_index] = visibility_calculator(candidate, perimeter);

            const bool is_too_visible{
                choice_visibilities[slice_index] >
                least_visible.visibility + params.jump_visibility_threshold};
            const bool can_be_on_edge{
                perimeter.angle_types[least_visible.choice.next_index] != AngleType::smooth};

            if (is_too_far || (can_be_on_edge && is_too_visible)) {
                candidate = least_visible.choice;
                reference_position = candidate.position;
            } else if (has_nearby_enforcers && !is_on_edge) {
                reference_position =
                    reference_position * (1.0 - blend_factor) + search_target * blend_factor;
                candidate.position = reference_position;
            } else {
                reference_position = candidate.position;
            }
            return candidate;
        })};

    if (choices.size() > 1) {
        constexpr double back_base_blend = 0.1;
        constexpr double back_ref_height = 0.1;
        Vec2d backward_ref = choices.back().position;
        for (std::size_t i = choices.size() - 1; i > 0; --i) {
            const std::size_t idx = i - 1;
            const Perimeter &perimeter = shell[idx].boundary;
            if (perimeter.is_degenerate)
                continue; // backward_ref intentionally not reset across degenerate gaps

            const double layer_h = (idx + 1 < shell.size()) ?
                (shell[idx + 1].boundary.slice_z - perimeter.slice_z) :
                back_ref_height;
            const double back_blend =
                std::clamp(back_base_blend * (layer_h / back_ref_height), back_base_blend, 0.5);

            const bool is_on_edge = choices[idx].previous_index == choices[idx].next_index &&
                perimeter.angle_types[choices[idx].next_index] != AngleType::smooth;
            const bool has_nearby_enforcers =
                get_enforcer_centroid_near(perimeter, backward_ref, params.max_detour).has_value();

            if (has_nearby_enforcers && !is_on_edge) {
                backward_ref =
                    backward_ref * (1.0 - back_blend) + choices[idx].position * back_blend;
                choices[idx].position = (choices[idx].position + backward_ref) * 0.5;
            } else {
                backward_ref = choices[idx].position;
            }
        }
    }

    return SeamCandidate{std::move(choices), std::move(choice_visibilities)};
}

} // namespace Slic3r::Boss
