#include "Slic3r/App/Plater/CameraGizmo.hpp"

namespace Slic3r::App::Plater {

GizmoActivationState CameraGizmo::on_mouse(const GizmoEventContext& ctx, bool only_active)
{
    const Platform::MouseEvent& event = ctx.mouse_event();
    const auto type = event.type();
    if (type == Platform::MouseEvent::Type::ButtonDown) {
        if (!ctx.pick_results().empty())
            return GizmoActivationState::Inactive;

        const bool pan = event.button() == Platform::MouseButton::Right ||
                         event.button() == Platform::MouseButton::Middle;
        m_state = pan ? State::Panning : State::Rotating;
        m_last_x = event.x();
        m_last_y = event.y();
    } else if (type == Platform::MouseEvent::Type::Move) {
        if (m_state == State::Inactive)
            return GizmoActivationState::Inactive;

        float delta_x = (event.x() - m_last_x) / ctx.screen_info().logical_width();
        float delta_y = (event.y() - m_last_y) / ctx.screen_info().logical_height();
        m_last_x = event.x();
        m_last_y = event.y();
        
        if (m_state == State::Rotating)
            update_rotation(delta_x, delta_y);
        else if (m_state == State::Panning)
            update_pan(delta_x, delta_y);

        return GizmoActivationState::Active;
    } else if (type == Platform::MouseEvent::Type::ButtonUp) {
        m_state = State::Inactive;
        return only_active ? GizmoActivationState::Done : GizmoActivationState::Inactive;
    } else if (type == Platform::MouseEvent::Type::Wheel) {
        update_zoom(event.wheel_delta_y());
        return GizmoActivationState::Done;
    }
    if (m_state == State::Inactive)
        return GizmoActivationState::Inactive;
    return only_active ? GizmoActivationState::Active : GizmoActivationState::Probing;
}

void CameraGizmo::update_pan(float delta_x, float delta_y)
{
    auto& scene = m_scene_provider.scene();
    auto& cam = scene.camera();
    const auto& model = cam.model();
    auto right = model.block<3, 1>(0, 0);
    auto up = model.block<3, 1>(0, 1);
    auto& trackball = scene.camera_trackball();

    double dist = trackball.cam_focal_dist();
    trackball.set_focal_point(trackball.cam_focal() + right * -delta_x * dist + up * delta_y * dist);
}

void CameraGizmo::update_zoom(float wheel_delta_y)
{
    auto& scene = m_scene_provider.scene();
    auto& trackball = scene.camera_trackball();
    double d = trackball.cam_focal_dist();
    d += -wheel_delta_y * 0.2f;
    trackball.set_focal_distance(d);
}

void CameraGizmo::update_rotation(float delta_x, float delta_y)
{
    auto& scene = m_scene_provider.scene();
    auto& trackball = scene.camera_trackball();
    trackball.add_azimuth_and_zenith(-delta_x * M_PI, -delta_y * M_PI);
}


}
