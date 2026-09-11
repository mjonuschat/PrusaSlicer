#pragma once

#include <type_traits>

#include "boss/foundation/BossRegistryCommon.hpp"
#include "boss/foundation/SeamVisibilityContext.hpp"

namespace Slic3r::Boss {

template<class Feature, class = void>
struct HasModifyVisibility : std::false_type {};

template<class Feature>
struct HasModifyVisibility<
    Feature,
    std::void_t<decltype(Feature::modify_visibility(
        std::declval<const SeamVisibilityContext &>()))>> : std::true_type {};

template<class... Features>
struct BossSeamVisibilityRegistry {
private:
    static_assert(boss_ids_are_unique<Features...>(),
                  "duplicate BOSS feature id in this registry's composition");

public:
    // Sequential fold: every feature that implements modify_visibility gets
    // a chance to bias the running scores, in composition order. Unlike
    // BossExtrusionRegistry's modify_flow, there is no return value -- each
    // feature mutates ctx.visibility in place, since copying a
    // 30000-sample vector per feature would be wasteful and only one
    // feature is expected to touch it in practice.
    static void modify_visibility(const SeamVisibilityContext &ctx)
    {
        (fold_modify_visibility<Features>(ctx), ...);
    }

private:
    template<class Feature>
    static void fold_modify_visibility(const SeamVisibilityContext &ctx)
    {
        if constexpr (HasModifyVisibility<Feature>::value)
            Feature::modify_visibility(ctx);
    }
};

} // namespace Slic3r::Boss
