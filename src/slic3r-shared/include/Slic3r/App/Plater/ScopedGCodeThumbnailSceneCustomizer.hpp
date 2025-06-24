#pragma once

#include "Slic3r/App/Scene/Camera.hpp"

namespace Slic3r::App::Scene {
class Scene;
class Node;
} // namespace Slic3r::App::Scene

namespace Slic3r::App::Render {
class Material;
} // namespace Slic3r::App::Render

namespace Slic3r::Domain {
struct BedInstance;
struct BedRef;
class Project;
} // namespace Slic3r::Domain

namespace Slic3r::App::Plater {

class ScopedGCodeThumbnailSceneCustomizer
{
public:
    ScopedGCodeThumbnailSceneCustomizer(Scene::Scene& scene, const Domain::Project& project, const Domain::BedInstance& bed_inst,
        const Domain::BedRef& bed_ref, Scene::CameraProjectionType camera_type);
    ~ScopedGCodeThumbnailSceneCustomizer();

    ScopedGCodeThumbnailSceneCustomizer(const ScopedGCodeThumbnailSceneCustomizer& other) = delete;
    ScopedGCodeThumbnailSceneCustomizer(ScopedGCodeThumbnailSceneCustomizer&& other) = delete;
    ScopedGCodeThumbnailSceneCustomizer& operator = (const ScopedGCodeThumbnailSceneCustomizer& other) = delete;
    ScopedGCodeThumbnailSceneCustomizer& operator = (ScopedGCodeThumbnailSceneCustomizer&& other) = delete;

private:
    Scene::Scene& m_scene;
    struct Cache
    {
        Scene::Transform camera_model;
        double camera_zoom;

        Vec3d trackball_target;
        Vec3d trackball_pivot;
        double trackball_azimuth;
        double trackball_zenith;
        double trackball_distance;
        Eigen::Quaterniond trackball_view_rotation;

        Eigen::AlignedBox3d shadows_aabb;
        bool background_enabled;

        bool shadows_enabled;
        bool ao_enabled;
        bool pbr_enabled;

        bool switch_camera_projection_type;

        std::vector<Scene::Node*> hidden_nodes;
        std::vector<std::pair<Scene::Node*, Render::Material>> materials;
    };
    Cache m_cache;
};

} // namespace Slic3r::App::Plater
