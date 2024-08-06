#include "MathUtils.hpp"

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
    const float f = 1.0f / std::tan(Geometry::deg2rad(fovy / 2));
    const float dist_z = near_z - far_z;
    Matrix4f ret;
    ret <<
        f/aspect, 0, 0, 0,
        0, f, 0, 0,
        0, 0, (far_z + near_z) / dist_z, 2 * far_z * near_z / dist_z,
        0, 0, -1, 0;
    return ret;
}


}
