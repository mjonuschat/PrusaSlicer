///|/ Copyright (c) Prusa Research 2022 Vojtěch Bubník @bubnikv
///|/ Copyright (c) BOSS 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_SurfaceLiftChecker_hpp_
#define slic3r_SurfaceLiftChecker_hpp_

#include <vector>

#include "../AABBTreeIndirect.hpp"
#include "libslic3r/libslic3r.h"

namespace Slic3r {

// Forward declarations.
class ExPolygon;
class Layer;

class SurfaceLiftChecker
{
public:
    bool is_over_top_surface(const Layer &layer, const Point &point);

private:
    // Last layer visited, for which a cache of top surfaces was created.
    const Layer                        *m_layer{nullptr};
    // Top surface expolygons, referencing data owned by m_layer->regions()->surfaces().
    std::vector<const ExPolygon*>       m_top_surfaces;
    // Search structure over top surfaces.
    using AABBTree = AABBTreeIndirect::Tree<2, coord_t>;
    AABBTree                            m_aabbtree;
};

} // namespace Slic3r

#endif // slic3r_SurfaceLiftChecker_hpp_
