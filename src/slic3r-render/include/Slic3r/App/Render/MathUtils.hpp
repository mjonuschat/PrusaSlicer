#pragma once

#include <libslic3r/Point.hpp>
#include "Slic3r/App/Render/Types.hpp"

namespace Slic3r::App::Render {

Matrix4f ortho(float left, float right, float bottom, float top, float near_z, float far_z);
Matrix4d ortho(double left, double right, double bottom, double top, double near_z, double far_z);

Matrix4f frustum(float left, float right, float bottom, float top, float near_z, float far_z);
Matrix4d frustum(double left, double right, double bottom, double top, double near_z, double far_z);

Matrix4f perspective(float fovy, float aspect, float near_z, float far_z);
Matrix4d perspective(double fovy, double aspect, double near_z, double far_z);

Matrix4f look_at(const Vec3f& eye, const Vec3f& center, const Vec3f& up);
Matrix4d look_at(const Vec3d& eye, const Vec3d& center, const Vec3d& up);

Vec2f viewport_transform(const Rect& viewport, const Vec3f& ndc_pos);
Vec2d viewport_transform(const Rect& viewport, const Vec3d& ndc_pos);

} // namespace Slic3r::App::Render
