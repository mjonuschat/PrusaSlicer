#pragma once

#include <libslic3r/Point.hpp>

namespace Slic3r::App::Scene {

class IRaycastNodeComponent {
public:
    virtual ~IRaycastNodeComponent() = default;

    virtual bool raycast(
        const Matrix4f& world, const Vec3f& ray_origin, const Vec3f& ray_direction, double& t) const = 0;
};

}
