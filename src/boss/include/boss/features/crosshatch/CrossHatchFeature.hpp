///|/ Copyright (c) BambuStudio 2023 Bambu Lab
///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <memory>
#include <string_view>

namespace Slic3r {
class Fill;
}

namespace Slic3r::Boss {

struct CrossHatchFeature {
    static constexpr int id = 10101;
    static constexpr std::string_view key = "crosshatch";
    static constexpr std::string_view label = "CrossHatch";
    static constexpr bool anchoring_eligible = false;

    static std::unique_ptr<Fill> create_fill();
    static bool use_bridge_flow();
};

} // namespace Slic3r::Boss
