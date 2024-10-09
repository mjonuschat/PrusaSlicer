#include "Slic3r/App/Scene/Camera.hpp"
#include "Slic3r/App/Render/MathUtils.hpp"
#include "Slic3r/Log.hpp"

namespace Slic3r::App::Scene {

Camera::Camera()
    : m_model(Transform::Identity()), m_projection_getter(new PerspectiveCameraProjectionGetter)
{}

void Camera::set_viewport(const Render::Rect& viewport)
{
    m_viewport = viewport;
    if (m_projection_getter)
        m_projection = m_projection_getter->projection(m_viewport);
}

void Camera::look_at(const Vec3f& eye, const Vec3f& center, const Vec3f& up)
{
    m_model = Render::look_at(eye, center, up).inverse();
}

Ray Camera::ray_at(float screen_x, float screen_y) const
{
#if 1
    screen_x -= m_viewport.x;
    screen_y -= m_viewport.y;

    Vec3f ray_nds{
        (2.0f * screen_x) / m_viewport.width - 1.0f,
        1.0f - (2.0f * screen_y) / m_viewport.height,
        1
    };
    Vec4f ray_clip{ray_nds.x(), ray_nds.y(), -1, 1};
    Vec4f ray_eye = m_projection.inverse() * ray_clip;
    ray_eye.z() = -1;
    ray_eye.w() = 0;

    Vec3f ray_world = (m_model * ray_eye).head<3>();

//    SPDLOG_INFO("ray NDS ({},  {},  {})", ray_nds.x(), ray_nds.y(), ray_nds.z());
//    SPDLOG_INFO("ray clip ({},  {},  {},  {})", ray_clip.x(), ray_clip.y(), ray_clip.z(), ray_clip.w());
//    SPDLOG_INFO("ray eye ({},  {},  {},  {})", ray_eye.x(), ray_eye.y(), ray_eye.z(), ray_eye.w());
//    SPDLOG_INFO("ray world ({},  {},  {})", ray_world.x(), ray_world.y(), ray_world.z());

    return {m_model.block<3, 1>(0, 3), ray_world.normalized()};
#else
    Vec3f p0 = unproject({screen_x, m_viewport.height - screen_y - 1, 0});
    Vec3f p1 = unproject({screen_x, m_viewport.height - screen_y - 1, 1});
    Vec3f dir = (p1 - p0).normalized();
    Vec3f o = m_model.block<3, 1>(0, 3);
//    SPDLOG_INFO("ray dir {} {} {}", dir.x(), dir.y(), dir.z());
//    SPDLOG_INFO("ray orig {} {} {}", o.x(), o.y(), o.z());
//    SPDLOG_INFO("ray p0 {} {} {}", p0.x(), p0.y(), p0.z());
//    SPDLOG_INFO("ray p1 {} {} {}", p1.x(), p1.y(), p1.z());
    return {o, dir};
#endif
}

Vec3f Camera::unproject(const Vec3f& win_pos) const
{
    Matrix4f inv_pm = (m_projection * view()).inverse();
    Vec4f w{
        (2 * win_pos.x() - m_viewport.x) / m_viewport.width - 1,
        (2 * win_pos.y() - m_viewport.y) / m_viewport.height - 1,
        2 * win_pos.z() - 1,
        1
    };
    Vec4f p = inv_pm * w;
    return p.head<3>() / p.w();
}

Transform PerspectiveCameraProjectionGetter::projection(const Render::Rect& viewport)
{
    return Render::perspective(m_fovy, float(viewport.width) / float(viewport.height), m_z_near, m_z_far);
}

}

