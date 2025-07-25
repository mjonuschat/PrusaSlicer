#include "Slic3r/App/Plater/QuickDragGizmo.hpp"
#include "Slic3r/App/Plater/SceneNodeTag.hpp"
#include "Slic3r/Domain/Types.hpp"

using Slic3r::Domain::SquareMatrix4d;
using Slic3r::Domain::Vec3d;

namespace Slic3r::App::Plater {
QuickDragGizmo::QuickDragGizmo(
    Biz::Scene::SceneInteractor& scene_interactor,
    Scene::ISceneProvider& scene_provider,
    const Scene::MouseDragDetector& drag_detector
) :
    m_scene_interactor(scene_interactor),
    m_scene_provider(scene_provider),
    m_selection_handler(scene_interactor),
    m_drag_detector(drag_detector)
{}

Scene::GizmoActivationState QuickDragGizmo::on_mouse(Scene::GizmoEventContext& ctx, bool only_active)
{
    Scene::DragState drag_state = m_drag_detector.get_state();
    switch (drag_state) {
    case Scene::DragState::no_drag:
        [[fallthrough]];
    case Scene::DragState::start_discard:
        return Scene::GizmoActivationState::Inactive;
    case Scene::DragState::start_we_will_see:
        return Scene::GizmoActivationState::Probing;
    case Scene::DragState::start: {
        // Additional condition to start dragging by QuickDrag
        const Scene::NodePickResult* n{nullptr};
        auto s_ctx = m_drag_detector.get_start()->create_ctx();
        if (s_ctx.mouse_event().key_modifiers() != 0
            || ctx.mouse_event().key_modifiers() != 0
            || (n = s_ctx.pick_result_with_tag_of_type<SceneNodeTag>()) == nullptr)
        {
            return Scene::GizmoActivationState::Inactive;
        }

        // Move the plane at same z-level as the node hit point is
        m_plane.d = -s_ctx.pick_ray().point_at(n->t).z();

        if (!mouse_pos(s_ctx.screen_mouse_x(), s_ctx.screen_mouse_y(), m_initial_world_pos)) {
            return Scene::GizmoActivationState::Inactive;
        }

        m_is_dragging = true;
        m_selection_handler.mark_selected(*n->node);
        return Scene::GizmoActivationState::Probing; // Scene::GizmoActivationState::Active
    }
    case Scene::DragState::dragging: {
        if (!m_is_dragging)
            return Scene::GizmoActivationState::Inactive;

        Vec3d p;
        if (!mouse_pos(ctx.screen_mouse_x(), ctx.screen_mouse_y(), p)) {
            // weird should not appear
            m_is_dragging = false;
            return Scene::GizmoActivationState::Inactive;
        }

        SquareMatrix4d xform    = SquareMatrix4d::Identity();
        xform.block<3, 1>(0, 3) = p - m_initial_world_pos;

        m_scene_interactor.transform_selection(xform, m_xform_memento);

        return Scene::GizmoActivationState::Active;
    }
    case Scene::DragState::finish:
        if (!m_is_dragging)
            return Scene::GizmoActivationState::Inactive;
        m_is_dragging = false;
        m_scene_interactor.finalize_transform_selection(m_xform_memento, false);
        return Scene::GizmoActivationState::Done;
    case Scene::DragState::interupted:
        if (m_is_dragging)
            m_scene_interactor.finalize_transform_selection(m_xform_memento, true);
        m_is_dragging = false;
        return Scene::GizmoActivationState::Inactive;
    default:
        SPDLOG_INFO(
            "  QuickDragGizmo reach unknown DragState: {} ({})",
            to_string(drag_state).c_str(),
            static_cast<int>(drag_state)
        );
        return Scene::GizmoActivationState::Inactive;
    }
}

void QuickDragGizmo::on_cycle_prepare()
{
    m_is_dragging = false;
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
