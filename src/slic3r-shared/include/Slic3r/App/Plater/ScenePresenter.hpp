#pragma once

#include <unordered_map>

#include "Slic3r/Domain/SelectionId.hpp"
#include "Slic3r/Domain/Workbench.hpp"
#include "Slic3r/Biz/ISelectedProjectChangedListener.hpp"
#include "Slic3r/Biz/ISelectedBedInstanceChangedListener.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/App/Plater/ScenePresenterProjectContext.hpp"
#include "Slic3r/App/Render/GeometryManager.hpp"
#include "Slic3r/App/Scene/TriangleMeshManager.hpp"
#include "Slic3r/App/Plater/ISceneProvider.hpp"
#include "Slic3r/App/Plater/BedRenderUpdater.hpp"

namespace Slic3r::App::Scene { class NodeBuilder; }

namespace Slic3r::App::Plater {

struct BedNodeTag;

class ScenePresenter : public Biz::ISelectedProjectChangedListener,
                       public Biz::Scene::ISceneSelectionChangedListener,
                       public Biz::ISelectedBedInstanceChangedListener,
                       public Biz::Scene::ISceneChangedListener,
                       public Scene::MinimalSceneRenderCustomizer,
                       public ISceneProvider
{
public:
    using ProjectContexts = std::unordered_map<Domain::SelectionId, ScenePresenterProjectContext>;
    using GeometryManager = Render::GeometryManager<std::string>;
    using TriangleMeshManager = Scene::TriangleMeshManager<std::string>;

    ScenePresenter(
        const Domain::Workbench& m_workbench,
        Biz::ProjectInteractor& project_interactor,
        Render::Device& device
    );


    bool project_ready() const { return !m_projects.empty(); }


    ScenePresenterProjectContext& project_context()
    {
        ASSERT(m_selected_project_id != Domain::INVALID_ID);
        return m_projects[m_selected_project_id];
    }

    const ScenePresenterProjectContext& project_context() const
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

    void update_beds() { m_bed_render_updater.update_all(); }

    Scene::Node& selection_root() override { return project_context().selection_root(); }

    void render_scene(Render::CommandBuffer& command_buffer);
    void render_imgui(const Render::ScreenInfo& screen_info);

    void screen_resized(const Render::Rect& viewport);

    void set_freeze_selection_center(bool freeze) { m_freeze_selection_center = freeze; }
    bool freeze_selection_center() const { return m_freeze_selection_center; }

    static double screen_space_sized_modifier() { return 0.0075; }

private:
    void update_cameras(const std::function<void(Scene::Camera&)>& modifier);

    void on_selected_project_changed(size_t index) override;

    void on_scene_selection_changed(Domain::SelectionId project_id, const Biz::Scene::Selection& selection) override;
    void on_scene_selection_transformed(Domain::SelectionId project_id, const Biz::Scene::Selection& selection) override;

    void on_selected_bed_instance_changed(Domain::SelectionId project_id, Domain::SelectionId container_id, Domain::SelectionId bed_instance_id) override;

    void on_instance_added(Domain::SelectionId project_id, const Domain::ElementRefs& instances) override;
    void on_instance_removed(Domain::SelectionId project_id, const Domain::ElementRefs& instances) override;
    void on_instance_transformed(Domain::SelectionId project_id, const Domain::ElementRefs& elements) override;

    void on_volume_added(Domain::SelectionId project_id, const Domain::ElementRefs& volumes) override;
    void on_volume_removed(Domain::SelectionId project_id, const Domain::ElementRefs& volumes) override;
    void on_volume_transformed(Domain::SelectionId project_id, const Domain::ElementRefs& elements) override;
    void on_volume_mesh_changed(Domain::SelectionId project_id, const Domain::ElementRefs& volumes) override;

    void on_bed_instance_added(Domain::SelectionId project_id, const Domain::BedRefs& instances) override;
    void on_bed_instance_removed(Domain::SelectionId project_id, const Domain::BedRefs& instances) override;
    void on_bed_instance_transformed(Domain::SelectionId project_id, const Domain::BedRefs& instances) override;

    void on_wipe_tower_added(Domain::SelectionId project_id, Domain::SelectionId  wipe_tower_id) override;
    void on_wipe_tower_removed(Domain::SelectionId project_id, Domain::SelectionId  wipe_tower_id) override;
    void on_wipe_tower_transformed(Domain::SelectionId project_id, Domain::SelectionId  wipe_tower_id) override;

    void on_layer_begin(Render::CommandBuffer& cmd_buf, size_t layer_idx) override;

    void build_volume_node(Scene::NodeBuilder& builder, Domain::SelectionId project_id, const ModelInstance* inst, const ModelVolume* vol);
    static Scene::Node* initialize_selection_root(Scene::Scene& scene);

    void build_bed_plate_node(Scene::NodeBuilder& builder, Domain::SelectionId project_id, const Domain::Bed& bed, const BedNodeTag& tag);
    void build_bed_grid_node(Scene::NodeBuilder& builder, Domain::SelectionId project_id, const Domain::Bed& bed, const BedNodeTag& tag);
    void build_bed_contour_node(Scene::NodeBuilder& builder, Domain::SelectionId project_id, const Domain::Bed& bed, const BedNodeTag& tag);
    void build_bed_print_volume_node(Scene::NodeBuilder& builder, Domain::SelectionId project_id, const Domain::Bed& bed, const BedNodeTag& tag);
    void build_bed_model_node(Scene::NodeBuilder& builder, Domain::SelectionId project_id, const Domain::Bed& bed, const BedNodeTag& tag);

    friend class ScenePresenterProjectContext;
private:
    const Domain::Workbench& m_workbench;
    Biz::ProjectInteractor& m_project_interactor;
    Render::Device& m_device;

    Domain::SelectionId m_selected_project_id{Domain::INVALID_ID};
    ProjectContexts m_projects;
    BedRenderUpdater m_bed_render_updater;

    bool m_freeze_selection_center{ false };
};

}
