///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <string_view>

namespace Slic3r::Domain {
class ConfigDefinitions;
}

namespace Slic3r::Boss {

struct AutoemitToolchangeCommandsFeature {
    static constexpr int id = 10007;
    static constexpr std::string_view key = "autoemit-toolchange-commands";
    static constexpr std::string_view label = "Disable automatic tool-change command emission";

    static void register_config(Domain::ConfigDefinitions& defs);
};

} // namespace Slic3r::Boss
