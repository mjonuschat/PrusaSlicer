///|/ Copyright (c) Prusa Research 2022 Vojtěch Bubník @bubnikv
///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
#pragma once

#include <vector>

#include "Slic3r/Biz/Algorithms/AABBTreeIndirect.hpp"
#include "Slic3r/Domain/ExPolygon.hpp"
#include "libslic3r/libslic3r.h"

namespace Slic3r {
class Layer;
class PrintObject;
} // namespace Slic3r

namespace Slic3r::Boss {

// Caches the top-surface expolygons of the last layer queried, keyed by Layer
// pointer identity plus the owning PrintObject pointer and the layer's own id,
// so repeated queries against the same layer only rebuild the AABB tree once.
// The instance is thread_local (see ZHopSurfaceFilterFeature::modify_retract),
// which outlives any single print: a reslice on the same worker thread can
// allocate a new Layer at an address the allocator already reused, so Layer*
// alone cannot tell two prints' layers apart. PrintObject* and layer id() are
// independent values that would also have to collide for the cache to alias.
// Not thread-safe by itself -- callers that query from multiple TBB worker
// threads must give each thread its own instance.
class SurfaceLiftChecker
{
public:
    bool is_over_top_surface(const Layer& layer, double x, double y);

private:
    friend struct SurfaceLiftCheckerCacheTestAccess;

    const Layer* m_layer{nullptr};
    const PrintObject* m_object{nullptr};
    size_t m_layer_id{0};
    std::vector<const Domain::ExPolygon*> m_top_surfaces;
    using AABBTree = Biz::Algorithms::AABBTreeIndirect::Tree<2, coord_t>;
    AABBTree m_aabbtree;
};

} // namespace Slic3r::Boss
