#pragma once

#include <string_view>

namespace Slic3r::Domain { class ConfigDefinitions; }

namespace Slic3r::Boss {

struct PerimeterOverlapFeature {
    static constexpr int id = 10103;
    static constexpr std::string_view key = "perimeter-overlap";
    static constexpr std::string_view label = "Configurable perimeter overlap";

    static void register_config(Domain::ConfigDefinitions &defs);
};

} // namespace Slic3r::Boss
