#pragma once

#include "libslic3r/ExPolygon.hpp"
#include "libslic3r/Point.hpp"

namespace Slic3r::Boss::NarrowSolidInfillErosion {

// True when `region` is narrow enough that morphological erosion at
// scaled_width * threshold_multiplier * 0.5 empties it out -- i.e. it
// should be routed to Arachne's variable-width ipEnsuring fill instead of
// the configured solid pattern.
bool classify(const ExPolygons &region, coord_t scaled_width, double threshold_multiplier);

// True when `region`'s total area is below sqr(scaled_width * threshold_multiplier) * 4.0
// AND it also erodes away per classify(). A large region can have a thin cross-section
// somewhere without being a narrow sliver overall, so both checks must pass together.
bool is_narrow_and_small(const ExPolygons &region, coord_t scaled_width, double threshold_multiplier);

} // namespace Slic3r::Boss::NarrowSolidInfillErosion
