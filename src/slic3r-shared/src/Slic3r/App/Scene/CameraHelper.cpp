#include "Slic3r/App/Scene/CameraHelper.hpp"
#include "Slic3r/App/Scene/Camera.hpp"

#include <cfloat>

namespace Slic3r::App::Scene {

void zoom_to_box(Camera& camera, const Eigen::AlignedBox3d& aabb)
{
    const AbstractCameraProjection& proj = camera.cam_projection();
    Transform3d view_xform = Transform3d(camera.view());

    if (proj.type() == CameraProjectionType::Perspective) {
        double max_angle_x = -DBL_MAX;
        double max_angle_y = -DBL_MAX;
        for (int i = 0; i < 8; ++i) {
            Vec3d eye_corner = view_xform * aabb.corner(Eigen::AlignedBox3d::CornerType(i));
            max_angle_x = std::max(max_angle_x, std::atan2(std::abs(eye_corner.x()), std::abs(eye_corner.z())));
            max_angle_y = std::max(max_angle_y, std::atan2(std::abs(eye_corner.y()), std::abs(eye_corner.z())));
        }

        // evaluate and apply the new zoom factor
        const Render::Rect& viewport = camera.viewport();
        double fov_y = deg2rad(0.5 * reinterpret_cast<const Scene::PerspectiveCameraProjection*>(&proj)->fovy());
        double fov_x = std::atan(double(viewport.width) / (double(viewport.height) * std::tan(fov_y)));
        camera.set_zoom(1.0 / std::max(max_angle_x / fov_x, max_angle_y / fov_y));
    }
    else {
        double max_x = -DBL_MAX;
        double max_y = -DBL_MAX;
        for (int i = 0; i < 8; ++i) {
            Vec3d eye_corner = view_xform * aabb.corner(Eigen::AlignedBox3d::CornerType(i));
            max_x = std::max(max_x, std::abs(eye_corner.x()));
            max_y = std::max(max_y, std::abs(eye_corner.y()));
        }
        double aabb_aspect_ratio = max_x / max_y;
        const Render::Rect& viewport = camera.viewport();
        double aspect_ratio = double(viewport.width) / double(viewport.height);
        double h = (aabb_aspect_ratio > aspect_ratio) ? max_x / aspect_ratio : max_y;
        static constexpr double MARGIN_FACTOR = 1.05f;
        camera.set_zoom(1.0 / (h * MARGIN_FACTOR));
    }
}

} // namespace Slic3r::App::Scene

