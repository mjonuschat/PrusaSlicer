#pragma once

#include <string_view>

namespace Slic3r::Domain { class ConfigDefinitions; }

namespace Slic3r::Boss {

struct SolidFillPolicyContext;

struct NarrowSolidInfillErosionFeature {
    static constexpr int id = 10201;
    static constexpr std::string_view key = "narrow-solid-infill-erosion";
    static constexpr std::string_view label = "Narrow solid-infill erosion";

    static void register_config(Domain::ConfigDefinitions &defs);

    static bool skip_narrow_top_bottom(const SolidFillPolicyContext &ctx);
    static bool force_ensuring(const SolidFillPolicyContext &ctx);
};

} // namespace Slic3r::Boss
