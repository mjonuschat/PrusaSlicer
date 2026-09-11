///|/ Copyright (c) Prusa Research 2022 Vojtěch Bubník @bubnikv
///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
#include "SurfaceLiftChecker.hpp"

#include "Slic3r/Biz/Algorithms/ExPolygon.hpp"
#include "boss/features/z-hop-surface-filter/ZHopSurfaceFilterFeature.hpp"
#include "boss/foundation/ExtrusionContext.hpp"
#include "libslic3r/ExPolygon.hpp"
#include "libslic3r/ExtrudeConfig.hpp"
#include "libslic3r/Layer.hpp"
#include "libslic3r/LayerRegion.hpp"
#include "libslic3r/Point.hpp"
#include "libslic3r/Surface.hpp"

namespace Slic3r::Boss {

bool SurfaceLiftChecker::is_over_top_surface(const Layer& layer, const double x, const double y)
{
    if (m_layer != &layer) {
        m_layer = &layer;
        m_top_surfaces.clear();
        m_aabbtree.clear();
        for (const LayerRegion* layerm : layer.regions())
            for (const Surface& surface : layerm->slices().surfaces)
                if (surface.is_top())
                    m_top_surfaces.emplace_back(&surface.expolygon);

        std::vector<Biz::Algorithms::AABBTreeIndirect::BoundingBoxWrapper> bboxes;
        bboxes.reserve(m_top_surfaces.size());
        for (size_t i = 0; i < m_top_surfaces.size(); ++i)
            bboxes.emplace_back(i, Biz::Algorithms::ExPolygon::get_extents(*m_top_surfaces[i]));
        m_aabbtree.build_modify_input(bboxes);
    }

    const Point point{scaled(x), scaled(y)};
    bool found = false;
    Biz::Algorithms::AABBTreeIndirect::traverse(
        m_aabbtree,
        [&point](const AABBTree::Node& node) { return node.bbox.contains(point); },
        [&point, &found, &surfaces = m_top_surfaces](const AABBTree::Node& node)
        {
            if (Biz::Algorithms::ExPolygon::contains(*surfaces[node.idx], point)) {
                found = true;
                return false;
            }
            return true;
        }
    );
    return found;
}

double ZHopSurfaceFilterFeature::modify_retract(double lift, const ExtrusionContext& ctx)
{
    if (lift <= 0.0 || ctx.layer == nullptr || ctx.extrude_config == nullptr)
        return lift;

    const ZHopSurfaceFilterMode mode =
        ctx.extrude_config->boss.retract_lift_enforce.at(ctx.extruder_id);
    if (mode == ZHopSurfaceFilterMode::AllSurfaces)
        return lift;

    static thread_local SurfaceLiftChecker checker;
    bool suppress = true;
    switch (mode) {
    case ZHopSurfaceFilterMode::TopOnly:
        suppress = !checker.is_over_top_surface(*ctx.layer, ctx.query_point_x, ctx.query_point_y);
        break;
    case ZHopSurfaceFilterMode::BottomOnly:
        suppress = !ctx.is_first_layer;
        break;
    case ZHopSurfaceFilterMode::TopAndBottom:
        suppress = !checker.is_over_top_surface(*ctx.layer, ctx.query_point_x, ctx.query_point_y)
            && !ctx.is_first_layer;
        break;
    default:
        suppress = false;
        break;
    }
    return suppress ? 0.0 : lift;
}

} // namespace Slic3r::Boss
