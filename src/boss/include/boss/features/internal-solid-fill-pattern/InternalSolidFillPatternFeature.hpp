///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <optional>
#include <string_view>

#include "Slic3r/Domain/ConfigDefsFDM.hpp"

namespace Slic3r::Domain { class ConfigDefinitions; }

namespace Slic3r::Boss {

struct SolidFillPolicyContext;

struct InternalSolidFillPatternFeature {
    static constexpr int id = 10501;
    static constexpr std::string_view key = "internal-solid-fill-pattern";
    static constexpr std::string_view label = "Internal solid-fill pattern";

    static void register_config(Domain::ConfigDefinitions &defs);

    static std::optional<Domain::InfillPattern> preferred_pattern(const SolidFillPolicyContext &ctx);
};

} // namespace Slic3r::Boss
