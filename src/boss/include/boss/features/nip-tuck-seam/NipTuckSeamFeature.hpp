// src/boss/include/boss/features/nip-tuck-seam/NipTuckSeamFeature.hpp
#pragma once

#include <optional>
#include <string_view>

#include "boss/foundation/PerimeterGeometryContext.hpp"

namespace Slic3r::Domain {
class ConfigDefinitions;
class ConfigView;
} // namespace Slic3r::Domain

namespace Slic3r::Boss {

enum class NipTuckSeamType { Regular, NipTuck, Nip, Tuck, Alternating };

struct NipTuckSeamFeature {
    static constexpr int id = 10508;
    static constexpr std::string_view key = "nip-tuck-seam";
    static constexpr std::string_view label = "Nip/Tuck (V-Notch) seam hiding";

    static void register_config(Domain::ConfigDefinitions &defs);
    static void modify_perimeters(const PerimeterGeometryContext &ctx);
    static bool suppress_staggering(const Domain::ConfigView &config, std::optional<int> perimeter_index);
};

} // namespace Slic3r::Boss
