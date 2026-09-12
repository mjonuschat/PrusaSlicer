#pragma once

#include <optional>
#include <type_traits>
#include <vector>

#include "boss/foundation/BossRegistryCommon.hpp"

namespace Slic3r { class PrintConfigView; }

namespace Slic3r::Boss {

// Detects whether Feature declares speed_cap() so a feature only implements
// it if it actually contributes a wipe tower speed limit.
template<class Feature, class = void>
struct HasSpeedCap : std::false_type {};

template<class Feature>
struct HasSpeedCap<
    Feature,
    std::void_t<decltype(Feature::speed_cap(
        std::declval<const Slic3r::PrintConfigView &>(),
        std::declval<const std::vector<unsigned> &>()
    ))>>
    : std::true_type {};

template<class... Features>
struct BossWipeTowerSpeedCapRegistry {
private:
    static_assert(boss_ids_are_unique<Features...>(),
                  "duplicate BOSS feature id in this registry's composition");

public:
    // Collects every feature's speed cap, if it has one. Order-independent --
    // WipeTower::apply_speed_caps() folds the result via std::min, so the
    // order caps are collected in has no effect on the outcome.
    static std::vector<float> collect(
        const Slic3r::PrintConfigView &config,
        const std::vector<unsigned> &extruder_candidates
    )
    {
        std::vector<float> caps;
        (fold_speed_cap<Features>(config, extruder_candidates, caps), ...);
        return caps;
    }

private:
    template<class Feature>
    static void fold_speed_cap(
        const Slic3r::PrintConfigView &config,
        const std::vector<unsigned> &extruder_candidates,
        std::vector<float> &caps
    )
    {
        if constexpr (HasSpeedCap<Feature>::value) {
            if (std::optional<float> cap = Feature::speed_cap(config, extruder_candidates))
                caps.push_back(*cap);
        }
    }
};

} // namespace Slic3r::Boss
