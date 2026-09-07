///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <optional>
#include <string_view>
#include <vector>

namespace Slic3r {
class PrintConfigView;
}

namespace Slic3r::Domain {
class ConfigDefinitions;
}

namespace Slic3r::Boss {

struct FilamentMaxSpeedFeature {
    static constexpr int id = 10002;
    static constexpr std::string_view key = "filament-max-speed";
    static constexpr std::string_view label = "Filament maximum speed";

    static void register_config(Domain::ConfigDefinitions& defs);
    static std::optional<float> speed_cap(
        const Slic3r::PrintConfigView& config,
        const std::vector<unsigned>& extruder_candidates
    );
};

} // namespace Slic3r::Boss
