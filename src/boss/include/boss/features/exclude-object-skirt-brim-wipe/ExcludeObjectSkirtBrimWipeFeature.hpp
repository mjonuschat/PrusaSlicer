#pragma once

#include <string_view>
#include <vector>

#include "libslic3r/boss/gcode/labeling/LabelObjectsPolicyContext.hpp"

namespace Slic3r::Boss {

struct ExcludeObjectSkirtBrimWipeFeature {
    static constexpr int id = 10512;
    static constexpr std::string_view key = "exclude-object-skirt-brim-wipe";
    static constexpr std::string_view label = "EXCLUDE_OBJECT for skirt/brim/wipe tower";

    static std::vector<LabelObjectsDeclaration> declare(const LabelObjectsPolicyContext &ctx);
};

} // namespace Slic3r::Boss
