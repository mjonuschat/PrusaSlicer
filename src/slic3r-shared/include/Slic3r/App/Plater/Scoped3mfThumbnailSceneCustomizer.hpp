#pragma once

#include "Slic3r/App/Scene/Camera.hpp"
#include "Slic3r/App/Scene/GraphicsSettings.hpp"
#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/App/Platform/CameraSynchData.hpp"

#include <optional>

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

class Scoped3mfThumbnailSceneCustomizer
{
public:
    Scoped3mfThumbnailSceneCustomizer(Scene::Scene& scene, const Domain::Project& project, Scene::CameraProjectionType camera_type);
    ~Scoped3mfThumbnailSceneCustomizer();

    Scoped3mfThumbnailSceneCustomizer(const Scoped3mfThumbnailSceneCustomizer& other) = delete;
    Scoped3mfThumbnailSceneCustomizer(Scoped3mfThumbnailSceneCustomizer&& other)      = delete;
    Scoped3mfThumbnailSceneCustomizer& operator=(const Scoped3mfThumbnailSceneCustomizer& other) = delete;
    Scoped3mfThumbnailSceneCustomizer& operator=(Scoped3mfThumbnailSceneCustomizer&& other) = delete;

private:
    Scene::Scene& m_scene;

    struct Cache
    {
        Platform::CameraSynchData camera_synch_data;
        Eigen::AlignedBox3d shadows_aabb;
        bool background_enabled;
        Scene::ShadingType shading_type;
        std::vector<Scene::Node*> hidden_nodes;
        std::vector<std::pair<Scene::Node*, std::optional<Render::Material>>> materials;
    };

    Cache m_cache;
};

} // namespace Slic3r::App::Plater
