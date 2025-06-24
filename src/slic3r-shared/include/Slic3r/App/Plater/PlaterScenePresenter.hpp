#pragma once

#include <unordered_map>

#include "Slic3r/Domain/SelectionId.hpp"
#include "Slic3r/Domain/Workbench.hpp"
#include "Slic3r/Biz/ISelectedProjectChangedListener.hpp"
#include "Slic3r/Biz/ISelectedBedInstanceChangedListener.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/App/Scene/ScenePresenterProjectContext.hpp"
#include "Slic3r/App/Render/GeometryManager.hpp"
#include "Slic3r/App/Scene/TriangleMeshManager.hpp"
#include "Slic3r/App/Scene/ISceneProvider.hpp"
#include "Slic3r/App/Scene/IProjectSceneProvider.hpp"
#include "Slic3r/App/Scene/BedRenderUpdater.hpp"
#include "Slic3r/App/Plater/IBedVisuallyChangedListener.hpp"

namespace Slic3r::App::Scene {
class NodeBuilder;
struct BedNodeTag;
} // namespace Slic3r::App::Scene

namespace Slic3r::App::Plater {

class PlaterScenePresenter : public WithListeners<Plater::IBedVisuallyChangedListener>,
                             public Biz::ISelectedProjectChangedListener,
                             public Biz::Scene::ISceneSelectionChangedListener,
                             public Biz::ISelectedBedInstanceChangedListener,
                             public Biz::Scene::ISceneChangedListener,
                             public Scene::MinimalSceneRenderCustomizer,
                             public Scene::ISceneProvider,
                             public Scene::IProjectSceneProvider
{
public:
    using ProjectContexts = std::unordered_map<Domain::SelectionId, Scene::ScenePresenterProjectContext>;
    using GeometryManager = Render::GeometryManager<std::string>;
    using TriangleMeshManager = Scene::TriangleMeshManager<std::string>;

    void load_selected_project();
    PlaterScenePresenter(
        const Domain::Workbench& m_workbench,
        Biz::ProjectInteractor& project_interactor,
        Render::Device& device
    );


    bool project_ready() const { return !m_projects.empty(); }

    Scene::ScenePresenterProjectContext& project_context()
    {
        ASSERT(m_selected_project_id != Domain::INVALID_ID);
        return m_projects[m_selected_project_id];
    }

    const Scene::ScenePresenterProjectContext& project_context() const
    {
        ASSERT(m_selected_project_id != Domain::INVALID_ID);
        return m_projects.find(m_selected_project_id)->second;
    }

    Scene::Scene& scene() override { return project_context().scene(); }
    const Scene::Scene& scene() const override { return project_context().scene(); }
    Scene::SceneChangeSession& selection_scene_changes() override
    {
        return project_context().selection_scene_changes();
    }

    void update_beds() { m_bed_render_updater.update_all(scene().camera()); }

    Scene::Node& selection_root() override { return project_context().selection_root(); }

    void render_scene(Render::CommandBuffer& command_buffer);
    void render_imgui(const Render::ScreenInfo& screen_info);

    void screen_resized(const Render::Rect& viewport);

    void set_freeze_selection_center(bool freeze) { m_freeze_selection_center = freeze; }
    bool freeze_selection_center() const { return m_freeze_selection_center; }

    double screen_space_sized_modifier() const {
        return project_context().screen_space_sized_modifier();
    }

    void update_objects_shadows_data();
    void update_beds_shadows_data();

    /**
     * @name Implementation of Scene::IProjectSceneProvider public interface
     * @{
     */
    Scene::Scene& project_scene(Domain::SelectionId project_id) override {
        return m_projects[project_id].scene();
    }

    const Scene::Scene& project_scene(Domain::SelectionId project_id) const override {
        return m_projects.find(project_id)->second.scene();
    }
    /**@}*/

    // At startup the scene initialization happens in the constructor, which means before any IBedVisuallyChangedListener
    // can be registered, see PlaterRenderModule::on_init()
    // Call this function to force bed thumbnails generation after the listeners are registered, for example to ensure
    // that the object list is properly updated
    void force_bed_thumbnails_generation();

private:
    void update_cameras(const std::function<void(Scene::Camera&)>& modifier);
    void update_camera_frustum();

    void on_selected_project_changed(size_t index) override;

    void on_scene_selection_changed(Domain::SelectionId project_id, const Biz::Scene::Selection& selection) override;
    void on_scene_selection_transformed(Domain::SelectionId project_id, const Biz::Scene::Selection& selection) override;

    void on_selected_bed_instance_changed(Domain::SelectionId project_id, Domain::SelectionId container_id, Domain::SelectionId bed_instance_id) override;

    void on_instance_added(Domain::SelectionId project_id, const Domain::ElementRefs& instances) override;
    void on_instance_removed(Domain::SelectionId project_id, const Domain::ElementRefs& instances) override;
    void on_instance_transformed(Domain::SelectionId project_id, const Domain::ElementRefs& elements, Biz::Scene::TransformState state) override;

    void on_volume_added(Domain::SelectionId project_id, const Domain::ElementRefs& volumes) override;
    void on_volume_removed(Domain::SelectionId project_id, const Domain::ElementRefs& volumes) override;
    void on_volume_transformed(Domain::SelectionId project_id, const Domain::ElementRefs& elements, Biz::Scene::TransformState state) override;
    void on_volume_mesh_changed(Domain::SelectionId project_id, const Domain::ElementRefs& volumes) override;

    void on_bed_instance_added(Domain::SelectionId project_id, const Domain::BedRefs& instances) override;
    void on_bed_instance_removed(Domain::SelectionId project_id, const Domain::BedRefs& instances) override;
    void on_bed_instance_transformed(Domain::SelectionId project_id, const Domain::BedRefs& instances, Biz::Scene::TransformState state) override;

    void on_wipe_tower_added(Domain::SelectionId project_id, Domain::SelectionId  wipe_tower_id) override;
    void on_wipe_tower_removed(Domain::SelectionId project_id, Domain::SelectionId  wipe_tower_id) override;
    void on_wipe_tower_transformed(Domain::SelectionId project_id, Domain::SelectionId  wipe_tower_id, Biz::Scene::TransformState state) override;

    void on_layer_begin(Render::CommandBuffer& cmd_buf, size_t layer_idx) override;

    void build_volume_node(Scene::NodeBuilder& builder, Domain::SelectionId project_id, const Domain::ModelInstance* inst, const Domain::ModelVolume* vol,
        std::optional<ColorRGBA> color = std::nullopt);

    const Domain::BedInstance& selected_bed_instance() const;

    void invoke_bed_visually_changed(Domain::SelectionId project_id);

private:
    const Domain::Workbench& m_workbench;
    Biz::ProjectInteractor& m_project_interactor;
    Render::Device& m_device;
    Render::Rect m_viewport;

    Domain::SelectionId m_selected_project_id{Domain::INVALID_ID};
    ProjectContexts m_projects;
    Scene::BedRenderUpdater m_bed_render_updater;

    bool m_freeze_selection_center{ false };
};

} // namespace Slic3r::App::Plater
