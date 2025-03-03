#include "Slic3r/App/Scene/AbstractCameraGizmo.hpp"
#include "Slic3r/App/Scene/Plane.hpp"


namespace Slic3r::App::Scene {

void AbstractCameraGizmo::register_commands(Platform::CommandRegistry& registry)
{
    registry
        .register_command(
            new Platform::FuncCommand(
                "zoom-in",
                [&]() { m_scene_provider.scene().camera_trackball().update_zoom(1.); },
                nullptr,
                Platform::KeyboardShortcut{0, Platform::KeyCode::I}
            ),
            true
        )
        .register_command(
            new Platform::FuncCommand(
                "zoom-out",
                [&]() { m_scene_provider.scene().camera_trackball().update_zoom(-1.); },
                nullptr,
                Platform::KeyboardShortcut{0, Platform::KeyCode::O}
            ),
            true
        )
        .register_command(
            new Platform::FuncCommand(
                "camera-projection-switch",
                [&]() { m_scene_provider.scene().camera_trackball().switch_projection_type(); },
                nullptr,
                Platform::KeyboardShortcut{0, Platform::KeyCode::K}
            ),
            true
        )
        .register_command(
            new Platform::FuncCommand(
                "look-at-active-bed",
                [&]() {
                    look_at(Vec3d(100, 100, 0), -M_PI_2 * 1.5, -M_PI_2 * 1.5);
                },
                nullptr,
                Platform::KeyboardShortcut{0, Platform::KeyCode::B}
            )
        )
    ;
}

// TODO: move these draw_* function into own module so they can be reused (+ add drawing-in-plane renderer utilizing x-axis and y-axis and origin on the plane)
template <typename V>
void draw_circle(Render::DynamicGeometry<V>& g, const Vec3f& position, const Vec3f& x_axis, const Vec3f& y_axis, float radius, size_t resolution = 32)
{
    auto builder = g.build_primitive(Render::PrimitiveType::LineLoop);
    for (size_t i = 0; i < resolution; i++) {
        auto phi = M_PI * 2 * i / resolution;
        Vec3f pt = x_axis * radius * std::cos(phi) + y_axis * radius * std::sin(phi) + position;
        builder.vertex(pt);
    }
}

template <typename V>
void draw_square(Render::DynamicGeometry<V>& g, const Vec3f& position, const Vec3f& x_axis, const Vec3f& y_axis, float radius)
{
    auto builder = g.build_primitive(Render::PrimitiveType::LineLoop);
    for (size_t i = 0; i < 4; i++) {
        const float sx = i / 2 == 0 ? -1 : 1;
        const float sy = i == 0 || i == 3 ? -1 : 1;
        Vec3f pt = x_axis * radius * sx + y_axis * radius * sy + position;
        builder.vertex(pt);
    }
}

template <typename V>
void draw_cross(Render::DynamicGeometry<V>& g, const Vec3f& position, const Vec3f& x_axis, const Vec3f& y_axis, float radius)
{
    auto builder = g.build_primitive(Render::PrimitiveType::Lines);
    for (size_t i = 0; i < 4; i++) {
        const float sx = i % 2 == 0 ? -1 : 1;
        const float sy = i == 0 || i == 3 ? -1 : 1;
        Vec3f pt = x_axis * radius * sx + y_axis * radius * sy + position;
        builder.vertex(pt);
    }
}

bool AbstractCameraGizmo::pick_plane(double mouse_x, double mouse_y, const Render::ScreenInfo& screen_info, Vec3d& out_plane_point)
{
    auto& scene = m_scene_provider.scene();

    auto& cam = scene.camera();
    auto r = cam.ray_at(screen_info.mouse_to_screen(mouse_x), screen_info.mouse_to_screen(mouse_y));

    Vec3d n = -cam.model().block<3, 1>(0, 2);
    Vec3d p = cam.model().block<3, 1>(0, 3);
    double q = p.dot(n) / n.dot(n);
    const Plane plane {
        n,
        -scene.camera_trackball().cam_focal_dist() - q
    };


    double t;
    // r.origin = Vec3d::Zero();
    if (plane.intersects(r, t)) {
        out_plane_point = r.point_at(t);
#if CAMERA_GIZMO_DEBUG
        Vec3d u, v;
        plane.vectors_in_plane(u, v);
        const size_t N{32};
        const double R{30};
        draw_circle(
            m_dynamic_geometry,
            out_plane_point.cast<float>(),
            u.cast<float>(), v.cast<float>(),
            R, N
        );
#endif
        return true;
    }
    return false;
}

GizmoActivationState AbstractCameraGizmo::on_mouse(GizmoEventContext& ctx, bool only_active)
{
#if CAMERA_GIZMO_DEBUG
    m_dynamic_geometry.clear();
    {
        const auto& scene = m_scene_provider.scene();
        const auto& cam = scene.camera();
        const auto& cam_trackball = scene.camera_trackball();

        draw_square(m_dynamic_geometry, cam_trackball.cam_focal().cast<float>(), cam.up().cast<float>(), cam.right().cast<float>(), 100);
        draw_cross(m_dynamic_geometry, cam_trackball.cam_focal().cast<float>(), cam.up().cast<float>(), cam.right().cast<float>(), 100);
    }
#endif

    const Platform::MouseEvent& event = ctx.mouse_event();
    const auto type = event.type();
    if (type == Platform::MouseEvent::Type::ButtonDown) {
        if (any_draggable(ctx))
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

        if (m_state == State::Rotating)
            update_rotation(delta_x, delta_y);
        else if (m_state == State::Panning) {
            Vec3d current_mouse_world_pos;
            Vec3d last_mouse_world_pos;
            if (pick_plane(event.x(), event.y(), ctx.screen_info(), current_mouse_world_pos) &&
                pick_plane(m_last_x, m_last_y, ctx.screen_info(), last_mouse_world_pos)) {
                for (size_t i = 0; i < 32; i++) {

                }
                update_pan(last_mouse_world_pos - current_mouse_world_pos);
            } else
                return GizmoActivationState::Inactive;
        }

        m_last_x = event.x();
        m_last_y = event.y();

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

void AbstractCameraGizmo::on_cycle_prepare()
{
    m_state = State::Inactive;
}

#if CAMERA_GIZMO_DEBUG
void AbstractCameraGizmo::render_scene(Render::CommandBuffer& cmd_buffer)
{
    if (!m_dynamic_geometry.empty()) {
        cmd_buffer.set_depth_test_enabled(true);
        m_dynamic_geometry.draw(cmd_buffer, Render::Material{}.set_shader(Render::Context::instance().shader_manager().get_shader("flat")));
    }
}
#endif


void AbstractCameraGizmo::update_pan(const Vec3d& delta)
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
}

void AbstractCameraGizmo::update_zoom(float wheel_delta_y)
{
    // On OSX with TrackPad when doing a small movement with two fingers (the scroll gesture)
    // the wheel_delta_y may be 0 (!) so prevent handling such events (this would lead to NaN in
    // zoom factor)
    if (wheel_delta_y != 0)
        m_scene_provider.scene().camera_trackball().update_zoom(
            wheel_delta_y / std::abs(wheel_delta_y)
        );
}

void AbstractCameraGizmo::update_rotation(float delta_x, float delta_y)
{
    auto& scene = m_scene_provider.scene();
    auto& trackball = scene.camera_trackball();
    trackball.add_azimuth_and_zenith(-delta_x * M_PI, -delta_y * M_PI);
}

void AbstractCameraGizmo::look_at(const Vec3d& pos, double azimuth, double zenith)
{
    auto& scene = m_scene_provider.scene();
    auto& trackball = scene.camera_trackball();
    trackball.set_focal_point(pos);
    trackball.set_azimuth_and_zenith(azimuth, zenith);
}

} // namespace Slic3r::App::Scene
