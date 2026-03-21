///|/ Copyright (c) Prusa Research 2022 Vojtěch Bubník @bubnikv
///|/ Copyright (c) BOSS 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "../Layer.hpp"
#include "SurfaceLiftChecker.hpp"
#include "libslic3r/AABBTreeIndirect.hpp"
#include "libslic3r/BoundingBox.hpp"
#include "libslic3r/ExPolygon.hpp"
#include "libslic3r/LayerRegion.hpp"
#include "libslic3r/Surface.hpp"

namespace Slic3r {

bool SurfaceLiftChecker::is_over_top_surface(const Layer &layer, const Point &point)
{
    if (m_layer != &layer) {
        // Update cache.
        m_layer = &layer;
        m_top_surfaces.clear();
        m_aabbtree.clear();
        // Collect expolygons of top surfaces.
        for (const LayerRegion *layerm : layer.regions())
            for (const Surface &surface : layerm->slices().surfaces)
                if (surface.is_top())
                    m_top_surfaces.emplace_back(&surface.expolygon);
        // Calculate bounding boxes of top surfaces.
        std::vector<AABBTreeIndirect::BoundingBoxWrapper> bboxes;
        bboxes.reserve(m_top_surfaces.size());
        for (size_t i = 0; i < m_top_surfaces.size(); ++i)
            bboxes.emplace_back(i, get_extents(*m_top_surfaces[i]));
        // Build AABB tree over bounding boxes of top surfaces.
        m_aabbtree.build_modify_input(bboxes);
    }

    AABBTree::BoundingBox point_bbox{point, point};
    point_bbox.extend(point + Point(SCALED_EPSILON, SCALED_EPSILON));
    point_bbox.extend(point - Point(SCALED_EPSILON, SCALED_EPSILON));
    bool found = false;
    AABBTreeIndirect::traverse(m_aabbtree,
        [&point_bbox](const AABBTree::Node &node) {
            return point_bbox.intersects(node.bbox);
        },
        [&point, &found, &surfaces = m_top_surfaces](const AABBTree::Node &node) {
            assert(node.is_leaf());
            assert(node.is_valid());
            if (surfaces[node.idx]->contains(point)) {
                found = true;
                // Stop traversal.
                return false;
            }
            // Continue traversal.
            return true;
        });
    return found;
}

} // namespace Slic3r
