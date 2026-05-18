///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Domain/Preset/SourceLocatedExpr.hpp"

namespace Slic3r::Domain::Preset {

struct ParsedExpr
{
    /**
     * @brief Source located simplified expression
     */
    SourceLocatedExpr expr;
    /**
     * @brief String representation of epxr
     */
    std::string expr_str;
};

} // namespace Slic3r::Domain::Preset
