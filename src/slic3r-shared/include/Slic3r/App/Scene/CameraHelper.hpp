#pragma once

#include <Slic3r/App/Render/Geometry.hpp>

namespace Slic3r::App::Scene {

class Camera;

void zoom_to_box(Camera& camera, const Eigen::AlignedBox3d& aabb);

} // namespace Slic3r::App::Scene

