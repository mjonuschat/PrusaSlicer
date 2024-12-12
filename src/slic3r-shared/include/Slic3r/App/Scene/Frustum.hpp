#pragma once

#include "Slic3r/App/Scene/Plane.hpp"

namespace Slic3r::App::Scene {

/**
 * @brief A generic (non necessarily orthogonal) frustum
*/
struct Frustum
{
    std::array<Vec3d, 8> vertices;
    std::array<Plane, 6> planes;

    /**
     * @brief Check if this frustum intersects the given axis aligned box
     * @param box The axis aligned box to test
     * @return True if this frustum and the given axis aligned box interect
     */
    bool intersects(const Eigen::AlignedBox3d& box) const;

    /**
     * @brief Check if this frustum intersects the sphere with the given center and radius
     * @param center The center of the sphere to test
     * @param radius The radius of the sphere to test
     * @return True if this frustum and the given sphere interect
     */
    bool intersects(const Vec3d& center, double radius) const;
};

} // namespace Slic3r::App::Scene
