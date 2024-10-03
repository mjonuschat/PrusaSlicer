#include "Slic3r/App/Scene/Camera.hpp"
#include "Slic3r/App/Render/MathUtils.hpp"

namespace Slic3r::App::Scene {
Camera::Camera() { set_perspective(90, 1, 0.1f, 100); }

void Camera::set_perspective(float fovy, float aspect, float near_z, float far_z)
{
    m_projection = Render::perspective(fovy, aspect, near_z, far_z);
}

void Camera::look_at(const Vec3f& eye, const Vec3f& center, const Vec3f& up)
{
    m_model = Render::look_at(eye, center, up);
}

}

