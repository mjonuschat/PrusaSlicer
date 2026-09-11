// src/boss/include/boss/foundation/BossPerimeterGeometryRegistry.hpp
#pragma once

#include <optional>
#include <type_traits>

#include "boss/foundation/BossRegistryCommon.hpp"
#include "boss/foundation/PerimeterGeometryContext.hpp"

namespace Slic3r::Domain {
class ConfigView;
}

namespace Slic3r::Boss {

template<class Feature, class = void>
struct HasModifyPerimeters : std::false_type {};

template<class Feature>
struct HasModifyPerimeters<
    Feature,
    std::void_t<decltype(Feature::modify_perimeters(std::declval<const PerimeterGeometryContext &>()))>>
    : std::true_type {};

template<class Feature, class = void>
struct HasSuppressStaggering : std::false_type {};

template<class Feature>
struct HasSuppressStaggering<
    Feature,
    std::void_t<decltype(Feature::suppress_staggering(
        std::declval<const Domain::ConfigView &>(), std::declval<std::optional<int>>()))>>
    : std::true_type {};

template<class... Features>
struct BossPerimeterGeometryRegistry {
private:
    static_assert(boss_ids_are_unique<Features...>(),
                  "duplicate BOSS feature id in this registry's composition");

public:
    // Sequential fold: every feature that implements modify_perimeters gets
    // a chance to mutate the vector in place (edit existing entries, append
    // new ones), in composition order.
    static void modify_perimeters(const PerimeterGeometryContext &ctx)
    {
        (fold_modify_perimeters<Features>(ctx), ...);
    }

    // OR-fold: any feature saying "suppress staggering here" wins.
    // Order-independent by construction -- no priority field needed here.
    static bool suppress_staggering(const Domain::ConfigView &config, std::optional<int> perimeter_index)
    {
        bool result = false;
        ((result = result || fold_suppress_staggering<Features>(config, perimeter_index)), ...);
        return result;
    }

private:
    template<class Feature>
    static void fold_modify_perimeters(const PerimeterGeometryContext &ctx)
    {
        if constexpr (HasModifyPerimeters<Feature>::value)
            Feature::modify_perimeters(ctx);
    }

    template<class Feature>
    static bool fold_suppress_staggering(const Domain::ConfigView &config, std::optional<int> perimeter_index)
    {
        if constexpr (HasSuppressStaggering<Feature>::value)
            return Feature::suppress_staggering(config, perimeter_index);
        else
            return false;
    }
};

} // namespace Slic3r::Boss
