#pragma once

#include "Slic3r/App/Scene/Transform.hpp"
#include "Slic3r/App/Scene/Camera.hpp"
#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/Domain/SelectionId.hpp"

#include <vector>

namespace Slic3r::App::Scene {
class Scene;
class Node;
} // namespace Slic3r::App::Scene

namespace Slic3r::App::Render {
class Material;
} // namespace Slic3r::App::Render

namespace Slic3r::Domain {
class Project;
} // namespace Slic3r::Domain

namespace Slic3r::App::Plater {

class ScopedBedThumbnailSceneCustomizer
{
public:
    ScopedBedThumbnailSceneCustomizer(
        Scene::Scene& scene,
        const Domain::Project& project,
        Domain::SelectionId bed_instance_id,
        Scene::CameraProjectionType camera_type
    );
    ~ScopedBedThumbnailSceneCustomizer();

    ScopedBedThumbnailSceneCustomizer(const ScopedBedThumbnailSceneCustomizer& other) = delete;
    ScopedBedThumbnailSceneCustomizer(ScopedBedThumbnailSceneCustomizer&& other)      = delete;
    ScopedBedThumbnailSceneCustomizer& operator=(const ScopedBedThumbnailSceneCustomizer& other) = delete;
    ScopedBedThumbnailSceneCustomizer& operator=(ScopedBedThumbnailSceneCustomizer&& other) = delete;

private:
    Scene::Scene& m_scene;

    struct Cache
    {
        Scene::Transform camera_model;
        double camera_zoom;

        Domain::Vec3d trackball_target;
        Domain::Vec3d trackball_pivot;
        double trackball_azimuth;
        double trackball_zenith;
        double trackball_distance;
        Eigen::Quaterniond trackball_view_rotation;

        Eigen::AlignedBox3d shadows_aabb;

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
