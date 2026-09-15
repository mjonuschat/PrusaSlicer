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

struct PrimeLengthAtStartFeature {
    static constexpr int id = 10008;
    static constexpr std::string_view key = "prime-length-at-start";
    static constexpr std::string_view label = "Prime length at start";

    static void register_config(Domain::ConfigDefinitions& defs);
};

} // namespace Slic3r::Boss
