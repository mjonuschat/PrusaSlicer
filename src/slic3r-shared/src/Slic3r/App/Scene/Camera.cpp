#include "Slic3r/App/Scene/Camera.hpp"
#include "Slic3r/App/Render/MathUtils.hpp"
#include "Slic3r/Log.hpp"
#include "libslic3r/Geometry.hpp"

namespace Slic3r::App::Scene {

Camera::Camera()
    : m_model(Transform::Identity()), m_projection_getter(new PerspectiveCameraProjection)
{}

void Camera::set_viewport(const Render::Rect& viewport)
{
    m_viewport = viewport;
    if (m_projection_getter)
        m_projection = m_projection_getter->projection(m_viewport);
    m_update_listeners.invoke([this](auto* l) { l->camera_updated(*this); });
}

void Camera::look_at(const Vec3d& eye, const Vec3d& center, const Vec3d& up)
{
    m_model = Render::look_at(eye, center, up).inverse();
    m_update_listeners.invoke([this](auto* l) { l->camera_updated(*this); });
}

void Camera::switch_projection_type()
{
    if (m_projection_getter->type() == CameraProjectionType::Perspective)
        m_projection_getter.reset(new OrthographicCameraProjection);
    else
        m_projection_getter.reset(new PerspectiveCameraProjection);

    set_viewport(m_viewport);
}

Ray Camera::ray_at(double screen_x, double screen_y) const
{
#if 1
    screen_x -= m_viewport.x;
    screen_y -= m_viewport.y;
    screen_y -= m_viewport.y;

    Vec3d ray_nds{
        (2.0 * screen_x) / m_viewport.width - 1.0,
        1.0 - (2.0 * screen_y) / m_viewport.height,
        1
    };
    Vec4d ray_clip{ray_nds.x(), ray_nds.y(), -1, 1};
    Vec4d ray_eye = m_projection.inverse() * ray_clip;
    if (m_projection_getter->type() == CameraProjectionType::Perspective) {
        ray_eye.z() = -1;
        ray_eye.w() = 0;

        Vec3d ray_world = (m_model * ray_eye).head<3>();

//        SPDLOG_INFO("ray NDS ({},  {},  {})", ray_nds.x(), ray_nds.y(), ray_nds.z());
//        SPDLOG_INFO("ray clip ({},  {},  {},  {})", ray_clip.x(), ray_clip.y(), ray_clip.z(), ray_clip.w());
//        SPDLOG_INFO("ray eye ({},  {},  {},  {})", ray_eye.x(), ray_eye.y(), ray_eye.z(), ray_eye.w());
//        SPDLOG_INFO("ray world ({},  {},  {})", ray_world.x(), ray_world.y(), ray_world.z());

        return {m_model.block<3, 1>(0, 3), ray_world.normalized()};
    }
    else {
        Vec4d ray_origin_eye(ray_eye.x(), ray_eye.y(), 0, 1);
        Vec4d ray_direction_eye = ray_eye - ray_origin_eye;
        ray_direction_eye.w() = 0;
        return {(m_model * ray_origin_eye).head<3>(), (m_model * ray_direction_eye).head<3>().normalized()};
    }
#else
    Vec3d p0 = unproject({screen_x, m_viewport.height - screen_y - 1, 0});
    Vec3d p1 = unproject({screen_x, m_viewport.height - screen_y - 1, 1});
    Vec3d dir = (p1 - p0).normalized();
    Vec3d o = m_model.block<3, 1>(0, 3);
//    SPDLOG_INFO("ray dir {} {} {}", dir.x(), dir.y(), dir.z());
//    SPDLOG_INFO("ray orig {} {} {}", o.x(), o.y(), o.z());
//    SPDLOG_INFO("ray p0 {} {} {}", p0.x(), p0.y(), p0.z());
//    SPDLOG_INFO("ray p1 {} {} {}", p1.x(), p1.y(), p1.z());
    return {o, dir};
#endif
}

Vec3d Camera::unproject(const Vec3d& win_pos) const
{
    Matrix4d inv_pm = (m_projection * view()).inverse();
    Vec4d w{
        (2 * win_pos.x() - m_viewport.x) / m_viewport.width - 1,
        (2 * win_pos.y() - m_viewport.y) / m_viewport.height - 1,
        2 * win_pos.z() - 1,
        1
    };
    Vec4d p = inv_pm * w;
    return p.head<3>() / p.w();
}

Transform PerspectiveCameraProjection::projection(const Render::Rect& viewport) const
{
    return Render::perspective(
        m_fovy, double(viewport.width) / double(viewport.height), m_z_near, m_z_far
    );
}

double PerspectiveCameraProjection::constant_screen_space_size_scale(
    const Camera& cam, double cam_object_dist
) const
{
    return cam_object_dist / (2 * std::tan(Geometry::deg2rad(m_fovy / 2)));
}

Transform OrthographicCameraProjection::projection(const Render::Rect& viewport) const
{
    double half_w = 0.5 * viewport.width;
    double half_h = 0.5 * viewport.height;
    return Render::ortho(-half_w, half_w, -half_h, half_h, m_z_near, m_z_far);
}

double OrthographicCameraProjection::constant_screen_space_size_scale(const Camera& cam, double cam_object_dist) const
{
    return 2 / (cam.viewport().width);
}

} // namespace Slic3r::App::Scene
