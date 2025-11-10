#pragma once

#include "Slic3r/App/Platform/CameraSynchData.hpp"
#include "Slic3r/App/Scene/GraphicsSettings.hpp"
#include "Slic3r/App/Render/Material.hpp"
#include "Slic3r/App/Scene/CameraFrustumUpdater.hpp"
#include "Slic3r/Domain/Types.hpp"

namespace Slic3r::App::Scene {
class Scene;
class Node;
} // namespace Slic3r::App::Scene

namespace Slic3r::Domain {
class Project;
struct BedRef;
struct BedInstance;
} // namespace Slic3r::Domain

namespace Slic3r::App::Plater {

class ScopedThumbnailSceneCustomizerBase
{
public:
    ScopedThumbnailSceneCustomizerBase(Scene::Scene& scene, const Domain::Project& project, Scene::CameraProjectionType camera_type)
      : m_scene(scene), m_project(project), m_camera_type(camera_type)
    {}
    virtual ~ScopedThumbnailSceneCustomizerBase();

    ScopedThumbnailSceneCustomizerBase(const ScopedThumbnailSceneCustomizerBase& other) = delete;
    ScopedThumbnailSceneCustomizerBase(ScopedThumbnailSceneCustomizerBase&& other) = delete;
    ScopedThumbnailSceneCustomizerBase& operator=(const ScopedThumbnailSceneCustomizerBase& other) = delete;
    ScopedThumbnailSceneCustomizerBase& operator=(ScopedThumbnailSceneCustomizerBase&& other) = delete;

protected:
    void store_shading_type();
    void store_background_enabled();
    void store_shadows_aabb();
    void store_camera_synch_data();

    void hide_gizmos();
    void hide_selection_aabb();
    void hide_non_part_volumes();
    void hide_non_selected_bed_instances(const Domain::BedRef& selected_bed_instance);
    void hide_volumes_outside_selected_bed_instances(const Domain::BedInstance& bed_instance);
    void hide_bed_accessories();
    void hide_non_printable_volumes();

    void disable_bed_override_material();
    void disable_volumes_override_material();
    void override_non_printable_volumes_material();
    void set_shadows();

    void switch_camera_projection_type();
    void set_background_enabled(bool enabled);
    void set_shading_type(Scene::ShadingType type);
    void set_camera_trackball(const Eigen::AlignedBox3d& aabb);
    void set_shadows_aabb(const Eigen::AlignedBox3d& aabb);
    void zoom_to_box(const Eigen::AlignedBox3d& aabb);
    void update_camera_frustum();

    Eigen::AlignedBox3d scene_aabb() const;
    Eigen::AlignedBox3d volume_parts_aabb() const;
    Eigen::AlignedBox3d bed_instance_aabb(const Domain::BedInstance& bed_instance) const;

protected:
    Scene::Scene& m_scene;
    const Domain::Project& m_project;
    Scene::CameraProjectionType m_camera_type;
    Scene::CameraFrustumUpdater m_camera_frustum_updater;

    struct Cache
    {
        std::vector<Scene::Node*> hidden_nodes;
        std::vector<std::pair<Scene::Node*, std::optional<Render::Material>>> materials;
        std::vector<std::pair<Scene::Node*, Render::Shadows>> shadows;
        bool background_enabled{ true };
        Scene::ShadingType shading_type{ Scene::ShadingType::Legacy };
        Eigen::AlignedBox3d shadows_aabb;
        Platform::CameraSynchData camera_synch_data;
    };

    Cache m_cache;
};

} // namespace Slic3r::App::Plater
