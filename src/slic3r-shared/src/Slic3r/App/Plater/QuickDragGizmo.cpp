#include "Slic3r/App/Plater/QuickDragGizmo.hpp"
#include "Slic3r/App/Plater/SceneNodeTag.hpp"

namespace Slic3r::App::Plater {

Scene::GizmoActivationState QuickDragGizmo::on_mouse(Scene::GizmoEventContext& ctx, bool only_active)
{
    const auto& e = ctx.mouse_event();
    const Scene::NodePickResult* n{nullptr};

    if (e.type() == Platform::MouseEvent::Type::ButtonDown) {
        if (e.key_modifiers() != 0 || e.button() != Platform::MouseButton::Left ||
            (n = ctx.pick_result_with_tag_of_type<SceneNodeTag>()) == nullptr) {
            m_state = State::Inactive;
            return Scene::GizmoActivationState::Inactive;
        }

        m_initial_mouse_pos = {e.x(), e.y()};
        // Move the plane at same z-level as the node hit point is
        m_plane.d = -ctx.pick_ray().point_at(n->t).z();

        if (!mouse_pos(ctx.screen_mouse_x(), ctx.screen_mouse_y(), m_initial_world_pos)) {
            m_state = State::Inactive;
            return Scene::GizmoActivationState::Inactive;
        }

        m_state = State::Probing;

        return Scene::GizmoActivationState::Probing;
    } else if (e.type() == Platform::MouseEvent::Type::Move) {
        if (m_state == State::Inactive)
            return Scene::GizmoActivationState::Inactive;

        if (m_state == State::Probing) {
            if ((n = ctx.pick_result_with_tag_of_type<SceneNodeTag>()) == nullptr) {
                m_state = State::Inactive;
                return Scene::GizmoActivationState::Inactive;
            }

            const int dist_sq = mouse_dist_sq(e.x(), e.y());
            if (dist_sq < THRESHOLD_DIST_SQ)
                return Scene::GizmoActivationState::Probing;
            SPDLOG_INFO("  QuickDragGizmo threshold reached {}", dist_sq);
            m_state = State::Dragging;
            m_selection_handler.mark_selected(*n->node);
        }

        // m_state == State::Dragging
        Vec3d p;
        if (!mouse_pos(ctx.screen_mouse_x(), ctx.screen_mouse_y(), p)) {
            m_state = State::Inactive;
            return Scene::GizmoActivationState::Inactive;
        }

        Matrix4d xform = Matrix4d::Identity();
        xform.block<3, 1>(0, 3) = p - m_initial_world_pos;

        m_scene_interactor.transform_selection(xform, m_xform_memento);

        return Scene::GizmoActivationState::Active;
    } else if (e.type() == Platform::MouseEvent::Type::ButtonUp) {
        if (m_state != State::Inactive)
            m_scene_interactor.finalize_transform_selection(m_xform_memento, false);

        const bool was_active = m_state == State::Dragging;
        m_state = State::Inactive;
        return was_active ? Scene::GizmoActivationState::Done : Scene::GizmoActivationState::Inactive;
    } else if (e.type() == Platform::MouseEvent::Type::Leave) {
        if (m_state != State::Inactive)
            m_scene_interactor.finalize_transform_selection(m_xform_memento, true);
        const bool was_active = m_state == State::Dragging;
        m_state = State::Inactive;
        return was_active ? Scene::GizmoActivationState::Done : Scene::GizmoActivationState::Inactive;
    } else {
        m_state = State::Inactive;
        return Scene::GizmoActivationState::Inactive;
    }
}

void QuickDragGizmo::on_cycle_prepare() { m_state = State::Inactive; }

bool QuickDragGizmo::mouse_pos(float screen_x, float screen_y, Vec3d& out_pos)
{
    const auto& cam = m_scene_provider.scene().camera();
    const auto ray = cam.ray_at(screen_x, screen_y);
    double t;
    if (m_plane.intersects(ray, t)) {
        out_pos = ray.point_at(t);
        return true;
    }
    return false;
}

} // namespace Slic3r::App::Plater
