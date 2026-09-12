///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "boss/features/internal-solid-fill-pattern/InternalSolidFillPatternFeature.hpp"
#include "libslic3r/boss/surface/SolidFillPolicyContext.hpp"
#include "libslic3r/ConfigViews.hpp"

namespace Slic3r::Boss {

std::optional<Domain::InfillPattern> InternalSolidFillPatternFeature::preferred_pattern(const SolidFillPolicyContext &ctx)
{
    return ctx.region_config.get<Domain::InfillPattern>("solid_fill_pattern");
}

} // namespace Slic3r::Boss
