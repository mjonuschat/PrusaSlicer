#pragma once

#include "Slic3r/App/Scene/Plane.hpp"

namespace Slic3r::App::Render {
class ScreenInfo;
struct Rect;
} // Slic3r::App::Scene

namespace Slic3r::App::Scene {

class Camera;

/**
 * @brief A generic (non necessarily orthogonal) frustum
*/
class Frustum
{
public: 
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

    /**
     * @brief Create frustum from a rectangle on screen
     * @param camera The camera in use
     * @param screen_info The screen resolution in use
     * @param rect The rectangle drawn on the screen
     * @return The frustum corresponding to the rectangle on screen
     *
     * @note
     * The frustum is delimited by six planes, with normals pointing inward, when the rectangle's width and height are non-zero.
     * If the rectangle degenerates to a segment, only three planes are defined.
     */
    static Frustum from(const Camera& camera, const Render::ScreenInfo& screen_info, const Render::Rect& rect);

    /**
     * @brief Create view frustum for the given camera
     * @param camera The camera in use
     * @param screen_info The screen resolution in use
     * @return The view frustum of the given camera
     */
    static Frustum from(const Camera& camera, const Render::ScreenInfo& screen_info);

private:
    std::vector<Vec3d> m_vertices;
    std::vector<Plane> m_planes;
};

} // namespace Slic3r::App::Scene
