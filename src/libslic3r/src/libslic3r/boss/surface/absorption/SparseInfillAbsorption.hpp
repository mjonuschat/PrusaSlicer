#pragma once

#include <vector>

#include "libslic3r/ExPolygon.hpp"
#include "libslic3r/Fill/Fill.hpp"

namespace Slic3r::Boss::SparseInfillAbsorption {

// Absorbs small stInternal (sparse) pockets that sit fully inside a solid
// fill into that solid fill, removes holes in stInternalSolid/
// stSolidOverBridge too small or too thin for sparse fill to cover, and
// consolidates stSolidOverBridge fragments that mark_as_infill_above_bridge()
// split by bridge angle. Mutates `surface_fills` in place; entries with no
// remaining expolygons are left in the vector (empty), matching group_fills()'s
// own convention of skipping empty SurfaceFill entries at dispatch time.
// `total_fill_boundary` is the union of every region's fill_expolygons() --
// the true innermost-perimeter boundary, used to keep merges from growing
// fill geometry into perimeter territory.
void absorb(std::vector<SurfaceFill> &surface_fills, const ExPolygons &total_fill_boundary);

} // namespace Slic3r::Boss::SparseInfillAbsorption
