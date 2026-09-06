// src/boss/include/boss/foundation/BossRegistryCommon.hpp
//
// Shared helpers for every BOSS registry foundation template. Foundation
// owns this file; it names no individual feature.
#pragma once

#include <cstddef>

namespace Slic3r::Boss {

// The generator's check_global_uniqueness() already rejects duplicate ids
// across manifests before it writes a composition. This check is a
// second, independent line of defense. It also catches a hand-edited
// generated file, or a registry instantiated without the generator.
template<class... Features>
constexpr bool boss_ids_are_unique()
{
    constexpr int ids[] = {Features::id..., 0};
    for (std::size_t i = 0; i < sizeof...(Features); ++i)
        for (std::size_t j = i + 1; j < sizeof...(Features); ++j)
            if (ids[i] == ids[j])
                return false;
    return true;
}

} // namespace Slic3r::Boss
