#pragma once

#include <string_view>

namespace Slic3r::Domain { class ConfigDefinitions; }

namespace Slic3r::Boss {

struct FirstTopLayerFlowRatioFeature {
    static constexpr int id = 10302;
    static constexpr std::string_view key = "first-top-layer-flow-ratio";
    static constexpr std::string_view label = "First/top layer flow ratio";

    static void register_config(Domain::ConfigDefinitions &defs);
};

} // namespace Slic3r::Boss
