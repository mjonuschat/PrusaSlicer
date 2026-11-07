///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
#pragma once

#include <string_view>

namespace Slic3r::Domain {
class ConfigDefinitions;
}

// Forward-declared, not included: this header is shared by the slic3r-domain-side
// config-registration source and the libslic3r-side call site, and
// SeamVisibilityContext includes libslic3r/Point.hpp, which slic3r-domain must
// not depend on -- forward-declare it here instead.
namespace Slic3r::Boss {
struct SeamVisibilityContext;
} // namespace Slic3r::Boss

namespace Slic3r::Boss {

struct AlignedRearSeamFeature {
    static constexpr int id = 10506;
    static constexpr std::string_view key = "aligned-rear-seam";
    static constexpr std::string_view label = "Aligned-rear seam placement";

    static void register_config(Domain::ConfigDefinitions &defs);
    static void modify_visibility(const SeamVisibilityContext &ctx);
};

} // namespace Slic3r::Boss
