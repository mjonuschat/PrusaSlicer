///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <string_view>

namespace Slic3r::Domain { class ConfigDefinitions; }
namespace Slic3r::Boss { struct PerimeterPolicyContext; }

namespace Slic3r::Boss {

struct AlternateExtraPerimeterFeature {
    static constexpr int id = 10101;
    static constexpr std::string_view key = "alternate-extra-perimeter";
    static constexpr std::string_view label = "Alternate extra perimeter";

    static void register_config(Domain::ConfigDefinitions &defs);
    static int  adjust_loop_count(int loop_count, const PerimeterPolicyContext &ctx);
};

} // namespace Slic3r::Boss
