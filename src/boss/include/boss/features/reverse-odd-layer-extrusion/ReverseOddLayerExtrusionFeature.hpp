///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <string_view>

namespace Slic3r::Domain { class ConfigDefinitions; }

namespace Slic3r::Boss {

struct ReverseOddLayerExtrusionFeature {
    static constexpr int id = 10105;
    static constexpr std::string_view key = "reverse-odd-layer-extrusion";
    static constexpr std::string_view label = "Reverse extrusion direction on odd layers";

    static void register_config(Domain::ConfigDefinitions &defs);
};

} // namespace Slic3r::Boss
