///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <string_view>

namespace Slic3r::Domain {
class ConfigDefinitions;
}

namespace Slic3r::Boss {

struct WipeTowerDisableRammingCoolingFeature {
    static constexpr int id = 10004;
    static constexpr std::string_view key = "wipe-tower-disable-ramming-cooling";
    static constexpr std::string_view label = "Disable wipe tower ramming and cooling";

    static void register_config(Domain::ConfigDefinitions& defs);
};

} // namespace Slic3r::Boss
