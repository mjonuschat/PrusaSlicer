#pragma once

#include <type_traits>

#include "boss/foundation/BossRegistryCommon.hpp"
#include "libslic3r/boss/perimeter/PerimeterPolicyContext.hpp"

namespace Slic3r::Boss {

// Detects whether Feature declares adjust_loop_count()/apply_ordering() so a
// feature only needs to implement the operations it actually contributes to.
// Mirrors the capability-detection idiom already used by BossFillRegistry's
// sibling registries -- do not add a virtual base class here.
template<class Feature, class = void>
struct HasAdjustLoopCount : std::false_type {};

template<class Feature>
struct HasAdjustLoopCount<
    Feature,
    std::void_t<decltype(Feature::adjust_loop_count(std::declval<int>(), std::declval<const PerimeterPolicyContext &>()))>>
    : std::true_type {};

template<class Feature, class = void>
struct HasApplyOrdering : std::false_type {};

template<class Feature>
struct HasApplyOrdering<
    Feature,
    std::void_t<decltype(Feature::apply_ordering(std::declval<OrderingPolicy &>(), std::declval<const PerimeterPolicyContext &>()))>>
    : std::true_type {};

template<class... Features>
struct BossPerimeterPolicyRegistry {
private:
    // Each feature's id doubles as its fold priority in this additive
    // registry -- two features touching the same OrderingPolicy field with
    // the same priority would make the fold order unspecified, so ids must
    // be unique the same way BossFillRegistry requires it for dispatch.
    static_assert(boss_ids_are_unique<Features...>(),
                  "duplicate BOSS feature id in this registry's composition");

public:
    static int adjust_loop_count(int loop_count, const PerimeterPolicyContext &ctx)
    {
        ((loop_count = fold_loop_count<Features>(loop_count, ctx)), ...);
        return loop_count;
    }

    static void apply_ordering(OrderingPolicy &policy, const PerimeterPolicyContext &ctx)
    {
        (fold_ordering<Features>(policy, ctx), ...);
    }

private:
    template<class Feature>
    static int fold_loop_count(int loop_count, const PerimeterPolicyContext &ctx)
    {
        if constexpr (HasAdjustLoopCount<Feature>::value)
            return Feature::adjust_loop_count(loop_count, ctx);
        else
            return loop_count;
    }

    template<class Feature>
    static void fold_ordering(OrderingPolicy &policy, const PerimeterPolicyContext &ctx)
    {
        if constexpr (HasApplyOrdering<Feature>::value)
            Feature::apply_ordering(policy, ctx);
    }
};

} // namespace Slic3r::Boss
