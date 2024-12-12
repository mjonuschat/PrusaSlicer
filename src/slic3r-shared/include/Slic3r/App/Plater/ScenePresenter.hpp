#pragma once

#include <unordered_map>

#include "Slic3r/Domain/SelectionId.hpp"
#include "Slic3r/Domain/Workbench.hpp"
#include "Slic3r/Biz/ISelectedProjectChangedListener.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/App/Plater/ScenePresenterProjectContext.hpp"
#include "Slic3r/App/Render/GeometryManager.hpp"
#include "Slic3r/App/Scene/TriangleMeshManager.hpp"
#include "Slic3r/App/Plater/ISceneProvider.hpp"

namespace Slic3r::App::Scene { class NodeBuilder; }

namespace Slic3r::App::Plater {

class ScenePresenter : public Biz::ISelectedProjectChangedListener,
                       public Biz::Scene::ISceneSelectionChangedListener,
                       public Biz::Scene::ISceneChangedListener,
                       public Scene::ISceneRenderCustomizer,
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

    GeometryManager& geometry_manager() { return m_geometry_manager; }
    TriangleMeshManager& triangle_mesh_manager() { return m_triangle_mesh_manager; }


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

    Scene::Node& selection_root() override { return project_context().selection_root(); }

    void render_scene(Render::CommandBuffer& command_buffer);
    void render_imgui(const Render::ScreenInfo& screen_info);

    void update_cameras(const std::function<void(Scene::Camera&)>& modifier);
private:
    void on_selected_project_changed(size_t index) override;

    void on_scene_selection_changed(Domain::SelectionId project_id, const Biz::Scene::Selection& selection) override;
    void on_scene_selection_transformed(Domain::SelectionId project_id, const Biz::Scene::Selection& selection) override;

    void on_instance_added(Domain::SelectionId project_id, const Domain::ElementRefs& instances) override;
    void on_instance_removed(Domain::SelectionId project_id, const Domain::ElementRefs& instances) override;
    void on_instance_transformed(Domain::SelectionId project_id, const Domain::ElementRefs& elements) override;

    void on_volume_added(Domain::SelectionId project_id, const Domain::ElementRefs& volumes) override;
    void on_volume_removed(Domain::SelectionId project_id, const Domain::ElementRefs& volumes) override;
    void on_volume_transformed(Domain::SelectionId project_id, const Domain::ElementRefs& elements) override;
    void on_volume_mesh_changed(Domain::SelectionId project_id, const Domain::ElementRefs& volumes) override;

    void on_bed_added(Domain::SelectionId project_id, size_t idx) override;
    void on_bed_removed(Domain::SelectionId project_id, size_t idx) override;
    void on_bed_transformed(Domain::SelectionId project_id, size_t idx) override;

    void on_wipe_tower_added(Domain::SelectionId project_id, size_t idx) override;
    void on_wipe_tower_removed(Domain::SelectionId project_id, size_t idx) override;
    void on_wipe_tower_transformed(Domain::SelectionId project_id, size_t idx) override;

    void on_layer_begin(Render::CommandBuffer& cmd_buf, size_t layer_idx) override;

    void build_volume_node(Scene::NodeBuilder& builder, Domain::SelectionId project_id, const ModelInstance* inst, const ModelVolume* vol);


private:
    const Domain::Workbench& m_workbench;
    Biz::ProjectInteractor& m_project_interactor;
    Render::Device& m_device;

    Domain::SelectionId m_selected_project_id{Domain::INVALID_ID};
    ProjectContexts m_projects;
    GeometryManager m_geometry_manager;
    TriangleMeshManager m_triangle_mesh_manager;

};

}
