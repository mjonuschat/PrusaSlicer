#include "Slic3r/App/Plater/CameraGizmo.hpp"
#include "Slic3r/App/Scene/Plane.hpp"

namespace Slic3r::App::Plater {

void CameraGizmo::register_commands(CommandRegistry& registry)
{
    registry
        .register_command(
            new FuncCommand(
                "zoom-in",
                [&]() { m_scene_provider.scene().camera_trackball().update_zoom(1.); },
                nullptr,
                KeyboardShortcut{0, Platform::KeyCode::I}
            ),
            true
        )
        .register_command(
            new FuncCommand(
                "zoom-out",
                [&]() { m_scene_provider.scene().camera_trackball().update_zoom(-1.); },
                nullptr,
                KeyboardShortcut{0, Platform::KeyCode::O}
            ),
            true
        )
        .register_command(
            new FuncCommand(
                "camera-projection-switch",
                [&]() { m_scene_provider.scene().camera_trackball().switch_projection_type(); },
                nullptr,
                KeyboardShortcut{0, Platform::KeyCode::K}
            ),
            true
        )
    ;
/*
    switch (e.code())
    {
    case Platform::KeyCode::I: // zoom in
    {
        m_scene_presenter->scene().camera_trackball().update_zoom(1.);
        break;
    }
    case Platform::KeyCode::K: // switch camera type
    {
        m_scene_presenter->scene().camera_trackball().switch_projection_type();
        break;
    }
    case Platform::KeyCode::O: // zoom out
    {
        m_scene_presenter->scene().camera_trackball().update_zoom(-1.);
        break;
    }
    */
}


bool CameraGizmo::pick_plane(const GizmoEventContext& ctx, Vec3d& out_plane_point)
{
    auto& scene = m_scene_provider.scene();
    auto& cam = scene.camera();
    const Scene::Plane plane {
        cam.model().block<3, 1>(0, 2),
        scene.camera_trackball().cam_focal_dist()
    };

    double t;
    auto r = ctx.pick_ray();
    r.origin = Vec3d::Zero();
    if (plane.intersects(r, t)) {
        out_plane_point = r.point_at(t);
        return true;
    }
    return false;
}

GizmoActivationState CameraGizmo::on_mouse(GizmoEventContext& ctx, bool only_active)
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
        if (pan && !pick_plane(ctx, m_mouse_last_world_position)) {
            // pick failed, we cannot pan
            return GizmoActivationState::Inactive;
        }
    } else if (type == Platform::MouseEvent::Type::Move) {
        if (m_state == State::Inactive)
            return GizmoActivationState::Inactive;

        float delta_x = (event.x() - m_last_x) / ctx.screen_info().logical_width();
        float delta_y = (event.y() - m_last_y) / ctx.screen_info().logical_height();
        m_last_x = event.x();
        m_last_y = event.y();
        
        if (m_state == State::Rotating)
            update_rotation(delta_x, delta_y);
        else if (m_state == State::Panning) {
            Vec3d current_mouse_world_pos;
            if (pick_plane(ctx, current_mouse_world_pos)) {
                update_pan(m_mouse_last_world_position - current_mouse_world_pos);
                m_mouse_last_world_position = current_mouse_world_pos;
            } else
                return GizmoActivationState::Inactive;
        }

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

void CameraGizmo::on_cycle_prepare()
{
    m_state = State::Inactive;
}

void CameraGizmo::update_pan(const Vec3d& delta)
{
    auto& scene = m_scene_provider.scene();
    // auto& cam = scene.camera();
    // const auto& model = cam.model();
    // auto right = model.block<3, 1>(0, 0);
    // auto up = model.block<3, 1>(0, 1);
    auto& trackball = scene.camera_trackball();

    // double dist = trackball.cam_focal_dist();
    // trackball.set_focal_point(trackball.cam_focal() + right * -delta_x * dist + up * delta_y * dist);
    trackball.set_focal_point(trackball.cam_focal() + delta);
    SPDLOG_DEBUG("Pan {},{},{}", delta.x(), delta.y(), delta.z());
}

void CameraGizmo::update_zoom(float wheel_delta_y)
{
    // On OSX with TrackPad when doing a small movement with two fingers (the scroll gesture)
    // the wheel_delta_y may be 0 (!) so prevent handling such events (this would lead to NaN in
    // zoom factor)
    if (wheel_delta_y != 0)
        m_scene_provider.scene().camera_trackball().update_zoom(
            wheel_delta_y / std::abs(wheel_delta_y)
        );
}

void CameraGizmo::update_rotation(float delta_x, float delta_y)
{
    auto& scene = m_scene_provider.scene();
    auto& trackball = scene.camera_trackball();
    trackball.add_azimuth_and_zenith(-delta_x * M_PI, -delta_y * M_PI);
}


}
