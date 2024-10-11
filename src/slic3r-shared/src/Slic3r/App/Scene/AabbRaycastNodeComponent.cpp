#include "Slic3r/App/Scene/AabbRaycastNodeComponent.hpp"
#include "Slic3r/App/Render/MathUtils.hpp"

namespace Slic3r::App::Scene
{

bool AabbRaycastNodeComponent::raycast(
    const Matrix4f& world, const Vec3f& ray_origin, const Vec3f& ray_direction, double& t
) const
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

Eigen::AlignedBox<float, 2> AabbRaycastNodeComponent::projected_bounding_box(
    const Matrix4f& mvp, const Render::Rect& viewport
) const
{
    ASSERT(m_aabb_mesh != nullptr);
    auto bbox3 = m_aabb_mesh->bounding_box();

    Eigen::AlignedBox<float, 2> ret;
    for (size_t i = 0; i < 8; i++) {
        Vec3f v = bbox3.corner(Eigen::AlignedBox<float, 3>::CornerType(i));
        Vec4f c4 = mvp * Vec4f{v.x(), v.y(), v.z(), 1};
        Vec3f c3 = Vec3f{
            c4.x() / c4.w(),
            c4.y() / c4.w(),
            c4.z() / c4.w()
        };
        ret.extend(Render::viewport_transform(viewport, c3));
    }

    return ret;
}



}
