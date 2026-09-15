// This feature joins no capability registry -- it has no config surface at
// all, so nothing generates its dispatch call, and no composition header
// ever references this trait. It exists only so the manifest has a
// syntactically valid trait/header pair; the algorithm itself lives in
// Slic3r::Boss::SparseInfillAbsorption::absorb(), called directly from
// Fill.cpp's group_fills().
#pragma once

#include <string_view>

namespace Slic3r::Boss {

struct SparseInfillAbsorptionFeature {
    static constexpr int id = 10202;
    static constexpr std::string_view key = "sparse-infill-absorption";
    static constexpr std::string_view label = "Sparse infill absorption";
};

} // namespace Slic3r::Boss
