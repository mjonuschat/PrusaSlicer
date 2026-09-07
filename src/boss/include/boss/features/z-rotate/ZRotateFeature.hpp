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

struct ZRotateFeature {
    static constexpr int id = 10003;
    static constexpr std::string_view key = "z-rotate";
    static constexpr std::string_view label = "Z-rotate on import";

    static void register_config(Domain::ConfigDefinitions& defs);
};

} // namespace Slic3r::Boss
