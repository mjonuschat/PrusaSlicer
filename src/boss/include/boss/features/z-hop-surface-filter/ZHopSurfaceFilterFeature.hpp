///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
#pragma once

#include <string_view>

namespace Slic3r::Domain {
class ConfigDefinitions;
} // namespace Slic3r::Domain

// Forward-declared, not included: this header is shared by the slic3r-domain-side
// config-registration source and the libslic3r-side call site, and ExtrusionContext
// lives under libslic3r, which slic3r-domain must not depend on.
namespace Slic3r::Boss {
struct ExtrusionContext;
} // namespace Slic3r::Boss

namespace Slic3r::Boss {

enum class ZHopSurfaceFilterMode
{
    AllSurfaces,
    TopOnly,
    BottomOnly,
    TopAndBottom
};

struct ZHopSurfaceFilterFeature
{
    static constexpr int id                 = 10505;
    static constexpr std::string_view key   = "z-hop-surface-filter";
    static constexpr std::string_view label = "Z-hop surface filtering";

    static void register_config(Domain::ConfigDefinitions& defs);
    static double modify_retract(double lift, const ExtrusionContext& ctx);
};

} // namespace Slic3r::Boss
