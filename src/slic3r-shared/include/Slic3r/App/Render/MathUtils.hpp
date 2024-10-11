#pragma once

#include <libslic3r/Point.hpp>
#include "Slic3r/App/Render/Types.hpp"

namespace Slic3r::App::Render {

Matrix4f ortho(float left, float right, float bottom, float top, float near_z, float far_z);
Matrix4f frustum(float left, float right, float bottom, float top, float near_z, float far_z);
Matrix4f perspective(float fovy, float aspect, float near_z, float far_z);
Matrix4f look_at(const Vec3f& eye, const Vec3f& center, const Vec3f& up);
Vec2f viewport_transform(const Rect& viewport, const Vec3f& ndc_pos);

} // namespace Slic3r::App::Render
