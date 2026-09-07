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

struct WipeTowerMaxPurgeSpeedFeature {
    static constexpr int id = 10005;
    static constexpr std::string_view key = "wipe-tower-max-purge-speed";
    static constexpr std::string_view label = "Wipe tower maximum purge speed";

    static void register_config(Domain::ConfigDefinitions& defs);
    static std::optional<float> speed_cap(
        const Slic3r::PrintConfigView& config,
        const std::vector<unsigned>& extruder_candidates
    );
};

} // namespace Slic3r::Boss
