// src/boss/include/boss/features/structured-fuzzy-skin/StructuredFuzzySkinFeature.hpp
#pragma once

#include <string_view>

namespace Slic3r::Domain {
class ConfigDefinitions;
}

namespace Slic3r::Domain::Boss {

// A plain new coEnum-equivalent, not a registry-adapter: unlike InfillPattern, no native enum exists to extend.
enum class FuzzySkinNoiseType {
    Classic,
    Perlin,
    Billow,
    RidgedMulti,
    Voronoi,
};

} // namespace Slic3r::Domain::Boss

namespace Slic3r::Boss {

struct StructuredFuzzySkinFeature {
    static constexpr int id = 10401;
    static constexpr std::string_view key = "structured-fuzzy-skin";
    static constexpr std::string_view label = "Structured fuzzy skin";

    static void register_config(Domain::ConfigDefinitions& defs);
};

} // namespace Slic3r::Boss
