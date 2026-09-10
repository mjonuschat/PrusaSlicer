#pragma once

#include <optional>
#include <type_traits>

#include "Slic3r/Domain/ConfigDefsFDM.hpp"
#include "boss/foundation/BossRegistryCommon.hpp"
#include "libslic3r/boss/surface/SolidFillPolicyContext.hpp"

namespace Slic3r::Boss {

// Detects whether Feature declares skip_narrow_top_bottom()/force_ensuring()/
// preferred_pattern() so a feature only needs to implement the operations it
// actually contributes. Mirrors the capability-detection idiom already used
// by BossPerimeterPolicyRegistry -- do not add a virtual base class here.
template<class Feature, class = void>
struct HasSkipNarrowTopBottom : std::false_type {};

template<class Feature>
struct HasSkipNarrowTopBottom<
    Feature,
    std::void_t<decltype(Feature::skip_narrow_top_bottom(std::declval<const SolidFillPolicyContext &>()))>>
    : std::true_type {};

template<class Feature, class = void>
struct HasForceEnsuring : std::false_type {};

template<class Feature>
struct HasForceEnsuring<
    Feature,
    std::void_t<decltype(Feature::force_ensuring(std::declval<const SolidFillPolicyContext &>()))>>
    : std::true_type {};

template<class Feature, class = void>
struct HasPreferredPattern : std::false_type {};

template<class Feature>
struct HasPreferredPattern<
    Feature,
    std::void_t<decltype(Feature::preferred_pattern(std::declval<const SolidFillPolicyContext &>()))>>
    : std::true_type {};

template<class... Features>
struct BossSolidFillPolicyRegistry {
private:
    static_assert(boss_ids_are_unique<Features...>(),
                  "duplicate BOSS feature id in this registry's composition");

public:
    // OR-fold: any feature saying "skip this narrow top/bottom sliver" wins.
    // Order-independent by construction -- no priority field needed here.
    static bool skip_narrow_top_bottom(const SolidFillPolicyContext &ctx)
    {
        bool result = false;
        ((result = result || fold_skip_narrow_top_bottom<Features>(ctx)), ...);
        return result;
    }

    // OR-fold: any feature saying "force Ensuring" wins. Order-independent.
    static bool force_ensuring(const SolidFillPolicyContext &ctx)
    {
        bool result = false;
        ((result = result || fold_force_ensuring<Features>(ctx)), ...);
        return result;
    }

    // First-decisive-wins fold. Only one feature is expected to implement
    // this at a time; a second contributor is a new design problem, not one
    // this registry papers over.
    static std::optional<Domain::InfillPattern> preferred_pattern(const SolidFillPolicyContext &ctx)
    {
        std::optional<Domain::InfillPattern> result;
        ((result = result.has_value() ? result : fold_preferred_pattern<Features>(ctx)), ...);
        return result;
    }

private:
    template<class Feature>
    static bool fold_skip_narrow_top_bottom(const SolidFillPolicyContext &ctx)
    {
        if constexpr (HasSkipNarrowTopBottom<Feature>::value)
            return Feature::skip_narrow_top_bottom(ctx);
        else
            return false;
    }

    template<class Feature>
    static bool fold_force_ensuring(const SolidFillPolicyContext &ctx)
    {
        if constexpr (HasForceEnsuring<Feature>::value)
            return Feature::force_ensuring(ctx);
        else
            return false;
    }

    template<class Feature>
    static std::optional<Domain::InfillPattern> fold_preferred_pattern(const SolidFillPolicyContext &ctx)
    {
        if constexpr (HasPreferredPattern<Feature>::value)
            return Feature::preferred_pattern(ctx);
        else
            return std::nullopt;
    }
};

} // namespace Slic3r::Boss
