#pragma once

#include "Slic3r/App/Scene/IRaycastNodeComponent.hpp"
#include "Slic3r/Assert.hpp"
#include <libslic3r/AABBMesh.hpp>

namespace Slic3r::App::Scene {

class AabbRaycastNodeComponent : public IRaycastNodeComponent {
public:
    explicit AabbRaycastNodeComponent(const AABBMesh* mesh)
        : m_aabb_mesh(mesh)
    {}

    bool raycast(
        const Matrix4f& world, const Vec3f& ray_origin, const Vec3f& ray_direction, double& t
    ) const override
    {
        ASSERT(m_aabb_mesh != nullptr);

        Matrix4f inv_world = world.inverse();

        Vec3d local_ray_origin = (inv_world * Vec4f{ray_origin.x(), ray_origin.y(), ray_origin.z(), 1}).head<3>().cast<double>();
        Vec3d local_ray_direction = (inv_world.block<3, 3>(0, 0) * ray_direction).cast<double>();

        auto hit = m_aabb_mesh->query_ray_hit(local_ray_origin, local_ray_direction);
        if (!hit.is_hit())
            return false;
        t = hit.distance();
        return true;
    }

private:
    const AABBMesh* m_aabb_mesh{nullptr};
};

}
