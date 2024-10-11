#include "Slic3r/App/Render/MathUtils.hpp"

#include <libslic3r/Geometry.hpp>


namespace Slic3r::App::Render {

Matrix4f ortho(float left, float right, float bottom, float top, float near_z, float far_z)
{
    Matrix4f ret;
    ret <<
        2 / (right - left), 0, 0, - (right + left) / (right - left),
        0, 2 / (top - bottom), 0, - (top + bottom) / (top - bottom),
        0, 0, - 2 / (far_z - near_z), - (far_z + near_z) / (far_z - near_z),
        0, 0, 0, 1;
    return ret;
}

Matrix4f frustum(float left, float right, float bottom, float top, float near_z, float far_z)
{
    const float inv_dx = 1.0f / (right - left);
    const float inv_dy = 1.0f / (top - bottom);
    const float inv_dz = 1.0f / (far_z - near_z);
    Matrix4f ret;
    ret <<
        2.0f * near_z * inv_dx, 0.0,    (left + right) * inv_dx,                             0.0,
        0.0, 2.0f * near_z * inv_dy,    (bottom + top) * inv_dy,                             0.0,
        0.0,                    0.0, -(near_z + far_z) * inv_dz, -2.0f * near_z * far_z * inv_dz,
        0.0,                    0.0,                       -1.0,                             0.0;
    return ret;
}

Matrix4f perspective(float fovy, float aspect, float near_z, float far_z)
{
    const float f = 1.0f / std::tan(Geometry::deg2rad(fovy) / 2);
    const float dist_z = near_z - far_z;
    Matrix4f ret;
    ret <<
        f/aspect, 0, 0, 0,
        0, f, 0, 0,
        0, 0, (far_z + near_z) / dist_z, 2 * far_z * near_z / dist_z,
        0, 0, -1, 0;
    return ret;
}

Matrix4f look_at(const Vec3f& eye, const Vec3f& center, const Vec3f& up)
{
    Vec3f f = (center - eye).normalized();
    Vec3f u = up.normalized();
    Vec3f s = f.cross(u).normalized();
    u = s.cross(f);

    Matrix4f ret = Matrix4f ::Identity();
    ret(0,0) = s.x();
    ret(0, 1) = s.y();
    ret(0, 2) = s.z();
    ret(0, 3) = -s.dot(eye);

    ret(1, 0) = u.x();
    ret(1, 1) = u.y();
    ret(1, 2) = u.z();
    ret(1, 3) = -u.dot(eye);

    ret(2, 0) = -f.x();
    ret(2, 1) = -f.y();
    ret(2, 2) = -f.z();
    ret(2, 3) = f.dot(eye);

    return ret;
}

Vec2f viewport_transform(const Rect& viewport, const Vec3f& ndc_pos)
{
    float viewport_half_width = viewport.width / 2.0f;
    float viewport_half_height = viewport.height / 2.0f;
    return {
        viewport_half_width * ndc_pos.x() + viewport.x + viewport_half_width,
        viewport_half_height * ndc_pos.y() + viewport.y + viewport_half_height
    };
}

}
