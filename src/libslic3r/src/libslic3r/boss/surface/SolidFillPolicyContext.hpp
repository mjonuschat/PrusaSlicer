#pragma once

#include "libslic3r/ExPolygon.hpp"
#include "libslic3r/libslic3r.h"

namespace Slic3r {
class PrintRegionConfigView;
} // namespace Slic3r

namespace Slic3r::Boss {

// Non-owning; the caller's group_fills() locals outlive every registry call
// made against this context -- do not store a SolidFillPolicyContext beyond
// that call.
struct SolidFillPolicyContext {
    const PrintRegionConfigView &region_config;
    const ExPolygons            &expolygons;
    coord_t                      flow_scaled_width;
};

} // namespace Slic3r::Boss
