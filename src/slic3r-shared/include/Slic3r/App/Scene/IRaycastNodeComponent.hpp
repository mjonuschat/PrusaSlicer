#pragma once

#include "Slic3r/App/Scene/Ray.hpp"
#include "Slic3r/App/Render/Types.hpp"

namespace Slic3r::App::Scene {

/**
 * @brief Generic interface for raycast collision detection component.
 * @see AabbRaycastNodeComponent
 */
class IRaycastNodeComponent {
public:
    virtual ~IRaycastNodeComponent() = default;

    /**
     * @brief raycast test given ray against collision object.
     * @param world World transform of associated node
     * @param ray Ray in world space
     * @param[out] t Output ray t of first hit (set only if `true` returned)
     * @return True if any collision hit was detected
     */
    virtual bool raycast(
        const Matrix4f& world, const Ray& ray, double& t) const = 0;

    /**
     * @brief Gets screen space projection of collision object as AABB
     * @param mvp Model-view-projection matrix
     * @param viewport Viewport
     * @return Projected bounding box in OpenGL screen space (i.e. coordinate  (0,0)
     * located in bottom left corner). Empty if object is outside z-range (i.e. behind camera
     * or too far from camera)
     */
    virtual Eigen::AlignedBox<float, 2> projected_bounding_box(
        const Matrix4f& mvp, const Render::Rect& viewport
    ) const = 0;
};

}
