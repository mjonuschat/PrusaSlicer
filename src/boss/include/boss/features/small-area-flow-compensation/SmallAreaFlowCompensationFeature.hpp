#pragma once

#include <string_view>

namespace Slic3r::Domain {
class ConfigDefinitions;
} // namespace Slic3r::Domain

// Forward-declared, not included: this header is shared by the slic3r-domain-side
// config-registration source and the libslic3r-side flow-compensator source, and
// ExtrusionContext lives under libslic3r, which slic3r-domain must not depend on.
namespace Slic3r::Boss {
struct ExtrusionContext;
} // namespace Slic3r::Boss

namespace Slic3r::Boss {

struct SmallAreaFlowCompensationFeature
{
    static constexpr int              id    = 10504;
    static constexpr std::string_view key   = "small-area-flow-compensation";
    static constexpr std::string_view label = "Small-area infill flow compensation";

    static void   register_config(Domain::ConfigDefinitions &defs);
    static double modify_flow(double dE, const ExtrusionContext &ctx);
};

} // namespace Slic3r::Boss
