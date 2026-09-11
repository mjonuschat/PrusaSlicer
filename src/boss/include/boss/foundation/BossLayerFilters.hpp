#pragma once

#include <string>
#include <type_traits>
#include <utility>

#include "boss/foundation/BossRegistryCommon.hpp"

namespace Slic3r::Boss {

// Detects whether Feature declares filter_layer(std::string) -> std::string.
// Mirrors the capability-detection idiom used by every other Boss registry.
template<class Feature, class = void>
struct HasFilterLayer : std::false_type {};

template<class Feature>
struct HasFilterLayer<Feature, std::void_t<decltype(Feature::filter_layer(std::declval<std::string>()))>>
    : std::true_type {};

template<class... Features>
struct BossLayerFilters {
private:
    static_assert(boss_ids_are_unique<Features...>(),
                  "duplicate BOSS feature id in this registry's composition");

public:
    // Sequential fold in declaration order: each feature's filter_layer()
    // receives the previous feature's output. A manifest priority field is
    // deferred until a second filter feature needs to run before or after
    // another -- YAGNI while only one contributor exists.
    static std::string filter_layer(std::string gcode)
    {
        ((gcode = fold_filter<Features>(std::move(gcode))), ...);
        return gcode;
    }

private:
    template<class Feature>
    static std::string fold_filter(std::string gcode)
    {
        if constexpr (HasFilterLayer<Feature>::value)
            return Feature::filter_layer(std::move(gcode));
        else
            return gcode;
    }
};

} // namespace Slic3r::Boss
