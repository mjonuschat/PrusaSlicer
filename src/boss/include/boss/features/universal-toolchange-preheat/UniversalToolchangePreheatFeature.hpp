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

struct UniversalToolchangePreheatFeature {
    static constexpr int id = 10009;
    static constexpr std::string_view key = "universal-toolchange-preheat";
    static constexpr std::string_view label = "Universal toolchange preheat";

    static void register_config(Domain::ConfigDefinitions& defs);
};

} // namespace Slic3r::Boss
