///|/ Copyright (c) preFlight 2026 oozeBot R&D @oozebot
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "libslic3r/boss/surface/narrow_solid/NarrowSolidInfillErosion.hpp"

#include <cmath>

#include "boss/features/narrow-solid-infill-erosion/NarrowSolidInfillErosionFeature.hpp"
#include "libslic3r/boss/surface/SolidFillPolicyContext.hpp"
#include "libslic3r/ClipperUtils.hpp"
#include "libslic3r/ConfigViews.hpp"
#include "libslic3r/libslic3r.h"

namespace Slic3r::Boss::NarrowSolidInfillErosion {

bool classify(const ExPolygons &region, coord_t scaled_width, double threshold_multiplier)
{
    const float erode_radius = float(scaled_width) * float(threshold_multiplier) * 0.5f;
    return opening_ex(region, erode_radius).empty();
}

bool is_narrow_and_small(const ExPolygons &region, coord_t scaled_width, double threshold_multiplier)
{
    double area = 0.0;
    for (const ExPolygon &expolygon : region)
        area += std::abs(expolygon.area());
    const double area_threshold = sqr(double(scaled_width) * threshold_multiplier) * 4.0;
    return area < area_threshold && classify(region, scaled_width, threshold_multiplier);
}

} // namespace Slic3r::Boss::NarrowSolidInfillErosion

namespace Slic3r::Boss {

bool NarrowSolidInfillErosionFeature::skip_narrow_top_bottom(const SolidFillPolicyContext &ctx)
{
    return ctx.region_config.get<bool>("detect_narrow_solid_infill")
        && NarrowSolidInfillErosion::is_narrow_and_small(ctx.expolygons, ctx.flow_scaled_width, 1.5);
}

bool NarrowSolidInfillErosionFeature::force_ensuring(const SolidFillPolicyContext &ctx)
{
    return ctx.region_config.get<bool>("detect_narrow_solid_infill")
        && NarrowSolidInfillErosion::is_narrow_and_small(
               ctx.expolygons, ctx.flow_scaled_width, ctx.region_config.get<double>("detect_narrow_solid_infill_threshold"));
}

} // namespace Slic3r::Boss
