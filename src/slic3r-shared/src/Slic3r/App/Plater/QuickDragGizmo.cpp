///|/ Copyright (c) Prusa Research 2025 Filip Sykala @Jony01
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Plater/QuickDragGizmo.hpp"
#include "Slic3r/App/Scene/SceneNodeTag.hpp"
#include "Slic3r/App/Plater/PlaterGizmosHelper.hpp"
#include "Slic3r/Domain/Types.hpp"

using Slic3r::Domain::SquareMatrix4d;
using Slic3r::Domain::Vec3d;
using Slic3r::App::Scene::SceneNodeTag;

namespace Slic3r::App::Plater {
QuickDragGizmo::QuickDragGizmo(
    Biz::Scene::SceneInteractor& scene_interactor,
    Scene::ISceneProvider& scene_provider
) :
    m_scene_interactor(scene_interactor),
    m_scene_provider(scene_provider),
    m_selection_handler(scene_interactor)
{}

Scene::GizmoActivationState QuickDragGizmo::on_mouse(Scene::GizmoEventContext& ctx, bool only_active)
{
    return Scene::GizmoActivationState::Inactive;
}

bool QuickDragGizmo::on_drag_start(const Scene::GizmoEventContext& ctx)
{
    // Additional condition to start dragging by QuickDrag
    const Scene::NodePickResult* n{nullptr};
    if (ctx.mouse_event().key_modifiers() != 0
        || (n = ctx.pick_result_with_tag_of_type<SceneNodeTag>()) == nullptr)
        return false;

    // Move the plane at same z-level as the node hit point is
    m_plane.d = -ctx.pick_ray().point_at(n->cast.distance).z();

    if (!mouse_pos(ctx.screen_mouse_x(), ctx.screen_mouse_y(), m_initial_world_pos))
        return false;

    if (!can_be_added_to_object_selection(*n->node, m_scene_interactor.object_selection()))
        return false;

    m_selection_handler.mark_selected(*n->node, true, true);
    return true;
}

bool QuickDragGizmo::on_dragging(const Scene::GizmoEventContext& ctx)
{
    Vec3d p;
    if (!mouse_pos(ctx.screen_mouse_x(), ctx.screen_mouse_y(), p)) {
        // weird should not appear
        return false;
    }

    SquareMatrix4d xform    = SquareMatrix4d::Identity();
    xform.block<3, 1>(0, 3) = p - m_initial_world_pos;

    m_scene_interactor.transform_selection(xform, m_xform_memento);
    return true;
}

void QuickDragGizmo::on_drag_finish()
{
    m_scene_interactor.finalize_transform_selection(m_xform_memento, false);
}

void QuickDragGizmo::on_drag_cancel()
{
    m_scene_interactor.finalize_transform_selection(m_xform_memento, true);
}

bool QuickDragGizmo::mouse_pos(float screen_x, float screen_y, Vec3d& out_pos)
{
    const auto& cam = m_scene_provider.scene().camera();
    const auto ray  = cam.ray_at(screen_x, screen_y);
    double t;
    if (m_plane.intersects(ray, t)) {
        out_pos = ray.point_at(t);
        return true;
    }
    return false;
}

} // namespace Slic3r::App::Plater
