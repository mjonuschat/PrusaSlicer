///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <string_view>

namespace Slic3r::Domain { class ConfigDefinitions; }

namespace Slic3r::Boss {

struct SmallPerimeterThresholdFeature {
    static constexpr int id = 10104;
    static constexpr std::string_view key = "small-perimeter-threshold";
    static constexpr std::string_view label = "Configurable small perimeter threshold";

    static void register_config(Domain::ConfigDefinitions &defs);
};

} // namespace Slic3r::Boss
