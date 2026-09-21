#ifndef libslic3r_FuzzySkin_hpp_
#define libslic3r_FuzzySkin_hpp_

#include "libslic3r/libslic3r.h"

namespace Slic3r::Arachne {
struct ExtrusionLine;
} // namespace Slic3r::Arachne

namespace Slic3r::PerimeterGenerator {
struct Parameters;
} // namespace Slic3r::PerimeterGenerator

namespace Slic3r::Feature::FuzzySkin {

void fuzzy_polygon(Polygon &polygon, double fuzzy_skin_thickness, double fuzzy_skin_point_distance,
                    const PrintRegionConfigView &config, double slice_z);

void fuzzy_extrusion_line(Arachne::ExtrusionLine &ext_lines, double fuzzy_skin_thickness, double fuzzy_skin_point_dist,
                           const PrintRegionConfigView &config, double slice_z);

bool should_fuzzify(const PrintRegionConfigView &config, size_t layer_idx, size_t perimeter_idx, bool is_contour);

Polygon apply_fuzzy_skin(const Polygon &polygon, const PrintRegionConfigView &base_config, const PerimeterRegions &perimeter_regions,
                          size_t layer_idx, size_t perimeter_idx, bool is_contour, double slice_z);

Arachne::ExtrusionLine apply_fuzzy_skin(const Arachne::ExtrusionLine &extrusion, const PrintRegionConfigView &base_config,
                                         const PerimeterRegions &perimeter_regions, size_t layer_idx, size_t perimeter_idx,
                                         bool is_contour, double slice_z);

} // namespace Slic3r::Feature::FuzzySkin

#endif // libslic3r_FuzzySkin_hpp_
