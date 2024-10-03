#pragma once

#include <libslic3r/Point.hpp>

namespace Slic3r::App::Render {

Matrix4f ortho(float left, float right, float bottom, float top, float near_z, float far_z);
Matrix4f frustum(float left, float right, float bottom, float top, float near_z, float far_z);
Matrix4f perspective(float fovy, float aspect, float near_z, float far_z);
Matrix4f look_at(const Vec3f& eye, const Vec3f& center, const Vec3f& up);

} // namespace Slic3r::App::Render
