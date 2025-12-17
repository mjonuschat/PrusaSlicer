///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Lukáš Matěna @lukasmatena
///|/ Copyright (c) BambuStudio 2023 manch1n @manch1n
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#pragma once

#include <optional>
#include <string>

#include "libslic3r/Print.hpp"
#include "libslic3r/Layer.hpp"

namespace Slic3r::Biz::Slicing {
struct ConflictResult
{
    std::string obj_name_1;
    std::string obj_name_2;
    float height{ 0.0f };
    const void* obj_1{ nullptr }; // nullptr means wipe tower
    const void* obj_2{ nullptr };

    void reset();
};

using ConflictResultOpt = std::optional<ConflictResult>;
ConflictResultOpt find_inter_of_lines_in_diff_objs(SpanOfConstPtrs<PrintObject> objs, const std::optional<WipeTowerData>& wipe_tower_data);

}
