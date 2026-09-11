#pragma once

#include <optional>
#include <type_traits>
#include <vector>

#include "boss/foundation/BossRegistryCommon.hpp"
#include "libslic3r/GCode/SeamAligned.hpp"
#include "libslic3r/GCode/SeamChoice.hpp"
#include "libslic3r/GCode/SeamShells.hpp"
#include "libslic3r/Point.hpp"

namespace Slic3r::Boss {

template<class Feature, class = void>
struct HasStartingPositionsOverride : std::false_type {};

template<class Feature>
struct HasStartingPositionsOverride<
    Feature,
    std::void_t<decltype(Feature::get_starting_positions_override(
        std::declval<const Seams::Shells::Shell<> &>(),
        std::declval<const Seams::Aligned::Params &>()))>>
    : std::true_type {};

template<class Feature, class = void>
struct HasSeamCandidateOverride : std::false_type {};

template<class Feature>
struct HasSeamCandidateOverride<
    Feature,
    std::void_t<decltype(Feature::get_seam_candidate_override(
        std::declval<const Seams::Shells::Shell<> &>(), std::declval<const Vec2d &>(),
        std::declval<const Seams::Aligned::SeamChoiceVisibility &>(),
        std::declval<const Seams::Aligned::Params &>(),
        std::declval<const std::vector<std::vector<double>> &>(),
        std::declval<const std::vector<Seams::Aligned::LeastVisiblePoint> &>()))>>
    : std::true_type {};

template<class Feature, class = void>
struct HasPostprocessShellChoices : std::false_type {};

template<class Feature>
struct HasPostprocessShellChoices<
    Feature,
    std::void_t<decltype(Feature::postprocess_shell_choices(
        std::declval<const Seams::Shells::Shell<> &>(),
        std::declval<std::vector<Seams::SeamChoice> &>(),
        std::declval<const Seams::Aligned::Params &>()))>>
    : std::true_type {};

template<class... Features>
struct BossSeamAlignedRegistry {
private:
    static_assert(boss_ids_are_unique<Features...>(),
                  "duplicate BOSS feature id in this registry's composition");

public:
    // First-decisive-wins fold, matching BossSolidFillPolicyRegistry::preferred_pattern.
    // Only one feature is expected to implement this at a time; a second
    // contributor is a new design problem, not one this registry papers over.
    static std::optional<std::vector<Vec2d>> get_starting_positions_override(
        const Seams::Shells::Shell<> &shell, const Seams::Aligned::Params &params)
    {
        std::optional<std::vector<Vec2d>> result;
        ((result = result.has_value() ? result : fold_starting_positions<Features>(shell, params)),
         ...);
        return result;
    }

    static std::optional<Seams::Aligned::SeamCandidate> get_seam_candidate_override(
        const Seams::Shells::Shell<> &shell,
        const Vec2d &starting_position,
        const Seams::Aligned::SeamChoiceVisibility &visibility_calculator,
        const Seams::Aligned::Params &params,
        const std::vector<std::vector<double>> &precalculated_visibility,
        const std::vector<Seams::Aligned::LeastVisiblePoint> &least_visible_points)
    {
        std::optional<Seams::Aligned::SeamCandidate> result;
        ((result = result.has_value() ? result :
              fold_seam_candidate<Features>(shell, starting_position, visibility_calculator, params,
                                             precalculated_visibility, least_visible_points)),
         ...);
        return result;
    }

    // Sequential fold: every feature that implements this gets a chance to
    // adjust the already-computed choices in place, in composition order.
    static void postprocess_shell_choices(
        const Seams::Shells::Shell<> &shell, std::vector<Seams::SeamChoice> &choices,
        const Seams::Aligned::Params &params)
    {
        (fold_postprocess<Features>(shell, choices, params), ...);
    }

private:
    template<class Feature>
    static std::optional<std::vector<Vec2d>> fold_starting_positions(
        const Seams::Shells::Shell<> &shell, const Seams::Aligned::Params &params)
    {
        if constexpr (HasStartingPositionsOverride<Feature>::value)
            return Feature::get_starting_positions_override(shell, params);
        else
            return std::nullopt;
    }

    template<class Feature>
    static std::optional<Seams::Aligned::SeamCandidate> fold_seam_candidate(
        const Seams::Shells::Shell<> &shell,
        const Vec2d &starting_position,
        const Seams::Aligned::SeamChoiceVisibility &visibility_calculator,
        const Seams::Aligned::Params &params,
        const std::vector<std::vector<double>> &precalculated_visibility,
        const std::vector<Seams::Aligned::LeastVisiblePoint> &least_visible_points)
    {
        if constexpr (HasSeamCandidateOverride<Feature>::value)
            return Feature::get_seam_candidate_override(
                shell, starting_position, visibility_calculator, params, precalculated_visibility,
                least_visible_points);
        else
            return std::nullopt;
    }

    template<class Feature>
    static void fold_postprocess(
        const Seams::Shells::Shell<> &shell, std::vector<Seams::SeamChoice> &choices,
        const Seams::Aligned::Params &params)
    {
        if constexpr (HasPostprocessShellChoices<Feature>::value)
            Feature::postprocess_shell_choices(shell, choices, params);
    }
};

} // namespace Slic3r::Boss
