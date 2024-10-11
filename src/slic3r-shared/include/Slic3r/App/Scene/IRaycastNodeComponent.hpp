#pragma once

#include <libslic3r/Point.hpp>
#include "Slic3r/App/Render/Types.hpp"

namespace Slic3r::App::Scene {

class IRaycastNodeComponent {
public:
    virtual ~IRaycastNodeComponent() = default;

    virtual bool raycast(
        const Matrix4f& world, const Vec3f& ray_origin, const Vec3f& ray_direction, double& t) const = 0;
    virtual Eigen::AlignedBox<float, 2> projected_bounding_box(
        const Matrix4f& mvp, const Render::Rect& viewport
    ) const = 0;
};

}
