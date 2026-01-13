///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Lukáš Matěna @lukasmatena
///|/ Copyright (c) BambuStudio 2023 manch1n @manch1n
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_ConflictChecker_hpp_
#define slic3r_ConflictChecker_hpp_

#include <queue>
#include <vector>
#include <optional>
#include <map>
#include <string>
#include <utility>

#include "Slic3r/Biz/Algorithms/Polyline.hpp"
#include "libslic3r/Print.hpp"
#include "libslic3r/ExtrusionEntity.hpp"
#include "libslic3r/ExtrusionRole.hpp"
#include "libslic3r/Layer.hpp"
#include "libslic3r/Line.hpp"
#include "libslic3r/Point.hpp"
#include "libslic3r/Polyline.hpp"

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
ConflictResultOpt find_inter_of_lines_in_diff_objs(SpanOfConstPtrs<PrintObject> objs, const WipeTowerData& wtd);

}

#endif
