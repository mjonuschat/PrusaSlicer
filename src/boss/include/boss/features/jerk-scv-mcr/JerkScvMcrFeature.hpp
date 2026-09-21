#pragma once

#include <optional>
#include <string_view>

namespace Slic3r::Domain {
class ConfigDefinitions;
} // namespace Slic3r::Domain

// Forward-declared, not included: this header is shared by the slic3r-domain-side
// config-registration source and the libslic3r-side policy source, and
// ExtrusionContext/MotionDynamics live under libslic3r, which slic3r-domain must
// not depend on.
namespace Slic3r::Boss {
struct ExtrusionContext;
struct MotionDynamics;
} // namespace Slic3r::Boss

namespace Slic3r::Boss {

struct JerkScvMcrFeature
{
    static constexpr int id               = 10501;
    static constexpr std::string_view key = "jerk-scv-mcr";
    static constexpr std::string_view label =
        "Per-feature jerk, square corner velocity, and minimum cruise ratio";

    static void register_config(Domain::ConfigDefinitions& defs);
    static std::optional<MotionDynamics> before_extrusion(const ExtrusionContext& ctx);
};

} // namespace Slic3r::Boss
