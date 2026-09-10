#pragma once

#include <string_view>

namespace Slic3r::Domain { class ConfigDefinitions; }

namespace Slic3r::Boss {

struct BridgeDensityFeature {
    static constexpr int id = 10301;
    static constexpr std::string_view key = "bridge-density";
    static constexpr std::string_view label = "Bridge density";

    static void register_config(Domain::ConfigDefinitions &defs);
};

} // namespace Slic3r::Boss
