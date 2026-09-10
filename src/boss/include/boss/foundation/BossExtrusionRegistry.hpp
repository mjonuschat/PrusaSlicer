#pragma once

#include <optional>
#include <type_traits>

#include "boss/foundation/BossRegistryCommon.hpp"
#include "boss/foundation/ExtrusionContext.hpp"
#include "boss/foundation/MotionDynamics.hpp"

namespace Slic3r::Boss {

template<class Feature, class = void>
struct HasBeforeExtrusion : std::false_type {};

template<class Feature>
struct HasBeforeExtrusion<
    Feature,
    std::void_t<decltype(Feature::before_extrusion(std::declval<const ExtrusionContext &>()))>>
    : std::true_type {};

template<class Feature, class = void>
struct HasModifyFlow : std::false_type {};

template<class Feature>
struct HasModifyFlow<
    Feature,
    std::void_t<decltype(Feature::modify_flow(std::declval<double>(), std::declval<const ExtrusionContext &>()))>>
    : std::true_type {};

template<class Feature, class = void>
struct HasModifyRetract : std::false_type {};

template<class Feature>
struct HasModifyRetract<
    Feature,
    std::void_t<decltype(Feature::modify_retract(std::declval<double>(), std::declval<const ExtrusionContext &>()))>>
    : std::true_type {};

template<class... Features>
struct BossExtrusionRegistry {
private:
    static_assert(boss_ids_are_unique<Features...>(),
                  "duplicate BOSS feature id in this registry's composition");

public:
    static std::optional<MotionDynamics> before_extrusion(const ExtrusionContext &ctx)
    {
        std::optional<MotionDynamics> result;
        ((result = result.has_value() ? result : fold_before_extrusion<Features>(ctx)), ...);
        return result;
    }

    static double modify_flow(double dE, const ExtrusionContext &ctx)
    {
        double result = dE;
        ((result = fold_modify_flow<Features>(result, ctx)), ...);
        return result;
    }

    static double modify_retract(double lift, const ExtrusionContext &ctx)
    {
        double result = lift;
        ((result = fold_modify_retract<Features>(result, ctx)), ...);
        return result;
    }

private:
    template<class Feature>
    static std::optional<MotionDynamics> fold_before_extrusion(const ExtrusionContext &ctx)
    {
        if constexpr (HasBeforeExtrusion<Feature>::value)
            return Feature::before_extrusion(ctx);
        else
            return std::nullopt;
    }

    template<class Feature>
    static double fold_modify_flow(double dE, const ExtrusionContext &ctx)
    {
        if constexpr (HasModifyFlow<Feature>::value)
            return Feature::modify_flow(dE, ctx);
        else
            return dE;
    }

    template<class Feature>
    static double fold_modify_retract(double lift, const ExtrusionContext &ctx)
    {
        if constexpr (HasModifyRetract<Feature>::value)
            return Feature::modify_retract(lift, ctx);
        else
            return lift;
    }
};

} // namespace Slic3r::Boss
