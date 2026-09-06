// src/boss/include/boss/foundation/BossConfigRegistry.hpp
//
// Foundation template for the config-registration extension family.
// Generated composition headers instantiate this with the traits of
// every feature that declares the "fdm_config" capability. Foundation
// owns this file; it names no individual feature.
#pragma once

#include "boss/foundation/BossRegistryCommon.hpp"

namespace Slic3r::Domain {
class ConfigDefinitions;
}

namespace Slic3r::Boss {

template<class... Features>
struct BossConfigRegistry {
private:
    static_assert(boss_ids_are_unique<Features...>(),
                  "duplicate BOSS feature id in this registry's composition");

public:
    static void register_config(Domain::ConfigDefinitions& defs)
    {
        (Features::register_config(defs), ...);
    }
};

} // namespace Slic3r::Boss
