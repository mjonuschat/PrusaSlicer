#pragma once

#include "Slic3r/App/Scene/Transform.hpp"
#include "Slic3r/App/Scene/Camera.hpp"
#include "Slic3r/App/Scene/GraphicsSettings.hpp"
#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/Domain/SelectionId.hpp"
#include "Slic3r/App/Platform/CameraSynchData.hpp"

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
    ScopedBedThumbnailSceneCustomizer(Scene::Scene& scene, const Domain::Project& project, Domain::SelectionId bed_instance_id, Scene::CameraProjectionType camera_type);
    ~ScopedBedThumbnailSceneCustomizer();

    ScopedBedThumbnailSceneCustomizer(const ScopedBedThumbnailSceneCustomizer& other) = delete;
    ScopedBedThumbnailSceneCustomizer(ScopedBedThumbnailSceneCustomizer&& other)      = delete;
    ScopedBedThumbnailSceneCustomizer& operator=(const ScopedBedThumbnailSceneCustomizer& other) = delete;
    ScopedBedThumbnailSceneCustomizer& operator=(ScopedBedThumbnailSceneCustomizer&& other) = delete;

private:
    Scene::Scene& m_scene;

    struct Cache
    {
        Platform::CameraSynchData camera_synch_data;
        Eigen::AlignedBox3d shadows_aabb;
        Scene::ShadingType shading_type;
        std::vector<Scene::Node*> hidden_nodes;
        std::vector<std::pair<Scene::Node*, Render::Material>> materials;
    };

    Cache m_cache;
};

} // namespace Slic3r::App::Plater
