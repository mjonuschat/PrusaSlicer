///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <string_view>

namespace Slic3r::Domain { class ConfigDefinitions; }
namespace Slic3r::Boss { struct PerimeterPolicyContext; struct OrderingPolicy; }

namespace Slic3r::Boss {

struct ExternalFirstHolesFeature {
    static constexpr int id = 10510;
    static constexpr std::string_view key = "external-first-holes";
    static constexpr std::string_view label = "External perimeters first for holes";

    static void register_config(Domain::ConfigDefinitions &defs);
    static void apply_ordering(OrderingPolicy &policy, const PerimeterPolicyContext &ctx);
};

} // namespace Slic3r::Boss
