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

struct PerObjectMultiplierFeature {
    static constexpr int id = 10001;
    static constexpr std::string_view key = "per-object-multiplier";
    static constexpr std::string_view label = "Per-object extrusion multiplier";

    static void register_config(Domain::ConfigDefinitions& defs);
};

} // namespace Slic3r::Boss
