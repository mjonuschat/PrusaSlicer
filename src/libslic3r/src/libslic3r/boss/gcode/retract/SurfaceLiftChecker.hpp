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
} // namespace Slic3r

namespace Slic3r::Boss {

// Caches the top-surface expolygons of the last layer queried, keyed by Layer
// pointer identity, so repeated queries against the same layer only rebuild
// the AABB tree once. Not thread-safe by itself -- callers that query from
// multiple TBB worker threads must give each thread its own instance.
class SurfaceLiftChecker
{
public:
    bool is_over_top_surface(const Layer& layer, double x, double y);

private:
    const Layer* m_layer{nullptr};
    std::vector<const Domain::ExPolygon*> m_top_surfaces;
    using AABBTree = Biz::Algorithms::AABBTreeIndirect::Tree<2, coord_t>;
    AABBTree m_aabbtree;
};

} // namespace Slic3r::Boss
