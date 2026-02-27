#pragma once

#include <unordered_map>

#include "Slic3r/App/Scene/GeometryDataFactory.hpp"
#include "Slic3r/Domain/Color.hpp"
#include "Slic3r/Domain/SelectionId.hpp"
#include "Slic3r/Domain/Workbench.hpp"
#include "Slic3r/Biz/ISelectedProjectChangedListener.hpp"
#include "Slic3r/Biz/ISelectedBedInstanceChangedListener.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/App/Plater/PlaterScenePresenterProjectContext.hpp"
#include "Slic3r/App/Render/GeometryManager.hpp"
#include "Slic3r/App/Scene/TriangleMeshManager.hpp"
#include "Slic3r/App/Scene/ISceneProvider.hpp"
#include "Slic3r/App/Scene/IProjectSceneProvider.hpp"
#include "Slic3r/App/Scene/BedRenderUpdater.hpp"
#include "Slic3r/App/Plater/IBedVisuallyChangedListener.hpp"
#include "Slic3r/App/Plater/ISelectionBoundingBoxChangedListener.hpp"
#include "Slic3r/App/Scene/CameraFrustumUpdater.hpp"
#include "Slic3r/App/Plater/QuickSelectGizmo.hpp"
#include "Slic3r/App/Scene/Camera.hpp"
#include "Slic3r/App/Scene/ISceneChangedListener.hpp"
#include "Slic3r/Biz/IProjectsChangedListener.hpp"
#include "Slic3r/Biz/FDMResultCache.hpp"
#include "Slic3r/Biz/SLAResultCache.hpp"

namespace Slic3r::App::Scene {
class NodeBuilder;
struct BedNodeTag;
} // namespace Slic3r::App::Scene

namespace Slic3r::App::Platform {
class AnimationManager;
} // namespace Slic3r::App::Platform

namespace Slic3r::App::Plater {

class PlaterScenePresenter :
    public WithListeners<Plater::IBedVisuallyChangedListener, ISelectionBoundingBoxChangedListener>,
    public Biz::ISelectedProjectChangedListener,
    public Biz::Scene::ISceneSelectionChangedListener,
    public Biz::ISelectedBedInstancesChangedListener,
    public Biz::Scene::ISceneChangedListener,
    public Scene::ISceneChangedListener,
    public Biz::Scene::ISceneBedInstanceChangedListener,
    public Scene::MinimalSceneRenderCustomizer,
    public Scene::ISceneProvider,
    public Scene::IProjectSceneProvider,
    public IHoverChangedListener,
    public Scene::ICameraUpdateListener,
    public Biz::IFDMResultCacheChangedListener,
    public Biz::ISLAResultCacheChangedListener,
    public Biz::IProjectsChangedListener
{
public:
    using ProjectContexts = std::unordered_map<Domain::SelectionId, PlaterScenePresenterProjectContext>;

    void load_selected_project();
    PlaterScenePresenter(
        const Domain::Workbench& m_workbench,
        Biz::ProjectInteractor& project_interactor,
        Render::Device& device,
        Platform::AnimationManager& animation_manager
    );

    bool project_ready() const { return !m_projects.empty(); }

    Scene::Scene& scene() override { return project_context().scene(); }
    const Scene::Scene& scene() const override { return project_context().scene(); }
    Scene::SceneChangeSession& selection_scene_changes() override
    {
        return project_context().selection_scene_changes();
    }

    using MeshManager = Scene::TriangleMeshManager<Scene::AuxiliaryElementId>;
    const MeshManager& model_trinagle_mesh_manager(Domain::SelectionId project_id = Domain::INVALID_ID) const {
        if (project_id == Domain::INVALID_ID)
            project_id = m_selected_project_id;
        auto it = m_projects.find(project_id);
        ASSERT(it != m_projects.end());
        return it->second.model_triangle_mesh_manager();
    }

    Scene::Node& selection_root() override
    {
        return project_context().selection_root;
    }

    Scene::Node& plain_selection_root() override
    {
        return project_context().plain_selection_root;
    }

    std::optional<Scene::OrientedBoundingBox> selection_bounding_box()
    {
        return project_context().selection_bounding_box;
    }

    std::optional<Scene::OrientedBoundingBox> selection_bounding_box() const
    {
        return project_context().selection_bounding_box;
    }

    void clear_selection_root_children();

    void render_scene(Render::CommandBuffer& command_buffer);
    void render_imgui(const Render::ScreenInfo& screen_info);

    void screen_resized(const Render::Rect& viewport);

    void set_freeze_selection_center(bool freeze) { m_freeze_selection_center = freeze; }
    bool freeze_selection_center() const { return m_freeze_selection_center; }

    void center_camera_on_selected_bed(bool animated);

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

    /**
     * @name Implementation of IHoverChangedListener public interface
     * @{
     */
    void on_hover_changed(const HoverData& hover_data) override;
    /**@}*/

    /**
     * @name Implementation of App::Scene::ISceneChangedListener public interface
     * @{
     */
    void on_node_added(Scene::Node* node) override;
    void on_node_removed(Scene::Node* node) override;
    void on_node_changed(Scene::Node* node) override;
    /**@}*/

    /**
     * @name Implementation of Scene::ICameraUpdateListener public interface
     * @{
     */
    void camera_updated(const Scene::Camera& cam) override { set_scene_aabb_as_dirty(); }
    /**@}*/

    /**
     * @name Partial implementation of Biz::IProjectsChangedListener public interface
     * @{
     */
    void on_project_loaded(Domain::SelectionId project_id) override;
    /**@}*/

    /**
     * @name Implementation of Biz::IFDMResultCacheChangedListener public interface
     * @{
     */
    void on_fdm_result_cache_changed(const Domain::SlicingId id) override;
    /**@}*/

    /**
     * @name Implementation of Biz::ISLAResultCacheChangedListener public interface
     * @{
     */
    void on_sla_result_cache_changed(const Domain::SlicingId& id) override;
    /**@}*/

    const std::optional<Platform::CameraSynchData>& camera_synch_data() const { return project_context().camera_synch_data(); }
    void set_camera_synch_data(const Platform::CameraSynchData& data) { project_context().set_camera_synch_data(data); }

    // At startup the scene initialization happens in the constructor, which means before any IBedVisuallyChangedListener
    // can be registered, see PlaterRenderModule::on_init()
    // Call this function to force bed thumbnails generation after the listeners are registered, for example to ensure
    // that the object list is properly updated
    void force_bed_thumbnails_generation();
    void update_bed_instances() { m_bed_render_updater.update_all(scene().camera(), project_context().bed_error()); }
    bool update_bed_instance_error_state(const Domain::SlicingId& id, bool error);

    void update_sinking_contours_visibility(const Platform::MouseEvent& e, const Render::ScreenInfo& screen_info);
    void set_sinking_contours_highlight_enabled(bool enable) { project_context().set_sinking_contours_highlight_enabled(enable); }

    std::shared_ptr<Scene::ModelGeometryProvider> model_geometry_provider() { return project_context().model_geometry_provider(); }

    using BedInstances = std::vector<std::reference_wrapper<const Domain::BedInstance>>;

private:
    void update_cameras(const std::function<void(Scene::Camera&)>& modifier);

    void set_scene_aabb_as_dirty() { m_camera_frustum_updater.set_scene_aabb_as_dirty(); }

    PlaterScenePresenterProjectContext& project_context()
    {
        ASSERT(m_selected_project_id != Domain::INVALID_ID);
        return m_projects[m_selected_project_id];
    }

    const PlaterScenePresenterProjectContext& project_context() const
    {
        ASSERT(m_selected_project_id != Domain::INVALID_ID);
        return m_projects.find(m_selected_project_id)->second;
    }

    void on_selected_project_changed(size_t index) override;

    void on_scene_selection_changed(Domain::SelectionId project_id, const Biz::Scene::ObjectSelection& selection) override;
    void on_scene_selection_transformed(Domain::SelectionId project_id, const Biz::Scene::ObjectSelection& selection) override;
    void on_scene_selection_reference_frame_changed(Domain::SelectionId project_id, const Biz::Scene::ObjectSelection& selection) override;

    void on_selected_bed_instances_changed(Domain::SelectionId project_id, const Biz::Scene::BedSelection& selection) override;

    void on_instance_added(Domain::SelectionId project_id, const Domain::ElementRefs& instances) override;
    void on_instance_removed(Domain::SelectionId project_id, const Domain::ElementRefs& instances) override;
    void on_instance_transformed(Domain::SelectionId project_id, const Domain::ElementRefs& elements, Biz::Scene::TransformState state,
        const Biz::BedTrackingChanges& bed_tracking_changes) override;

    void on_volume_added(Domain::SelectionId project_id, const Domain::ElementRefs& volumes) override;
    void on_volume_removed(Domain::SelectionId project_id, const Domain::ElementRefs& volumes) override;
    void on_volume_transformed(Domain::SelectionId project_id, const Domain::ElementRefs& elements, Biz::Scene::TransformState state,
        const Biz::BedTrackingChanges& bed_tracking_changes) override;

    void on_bed_instance_updated(Domain::SelectionId project_id, const Domain::BedRefs& instances) override;
    void on_bed_instance_removed(Domain::SelectionId project_id, const Domain::BedRefs& instances) override;
    void on_bed_instance_transformed(Domain::SelectionId project_id, const Domain::BedRefs& instances, Biz::Scene::TransformState state) override;

    void on_wipe_tower_changed(
        Domain::SlicingId slicing_id,
        const Biz::Print::WipeTowerGeometry& wipe_tower
    ) override;
    void on_wipe_tower_moved(Domain::SlicingId slicing_id) override;
    void on_wipe_tower_removed(Domain::SlicingId slicing_id) override;
    void update_wipe_tower_obb(std::size_t project_id);

    void on_layer_begin(Render::CommandBuffer& cmd_buf, Scene::RenderLayerId layer_idx) override;
    void on_opaque_pass_begin(Render::CommandBuffer& cmd_buf, Scene::RenderLayerId layer_idx) override;
    void on_transparent_pass_begin(Render::CommandBuffer& cmd_buf, Scene::RenderLayerId layer_idx) override;

    void build_volume_node(Scene::NodeBuilder& builder, Domain::SelectionId project_id, const Domain::ModelInstance* inst, const Domain::ModelVolume* vol,
        std::optional<Domain::ColorRGBA> color = std::nullopt);

    void build_unknown_wipe_tower_node(
        Scene::NodeBuilder& builder,
        const Biz::Print::WipeTowerGeometry& wipe_tower,
        Domain::SlicingId slicing_id
    );
    void build_wipe_tower_node(
        Scene::NodeBuilder& builder,
        const Biz::Print::WipeTowerGeometry& wipe_tower,
        Domain::SlicingId slicing_id
    );

    BedInstances selected_bed_instances() const;

    void invoke_bed_visually_changed(Domain::SelectionId project_id);


    Scene::OrientedBoundingBox get_instance_obb(
        Domain::SelectionId project_id,
        const Biz::Scene::ObjectSelection& selection
    );

    Scene::OrientedBoundingBox get_volume_obb(
        Domain::SelectionId project_id,
        const Biz::Scene::ObjectSelection& selection
    );

    Scene::OrientedBoundingBox get_global_obb(
        Domain::SelectionId project_id,
        const Biz::Scene::ObjectSelection& selection
    );

    void update_selection_obb(
        Domain::SelectionId project_id,
        const Biz::Scene::ObjectSelection& selection
    );

    void remove_beds(Domain::SelectionId project_id, const Domain::BedRefs& instances);
    void update_volume_materials();

private:
    const Domain::Workbench& m_workbench;
    Biz::ProjectInteractor& m_project_interactor;
    Render::Device& m_device;
    Render::Rect m_viewport;

    Domain::SelectionId m_selected_project_id{Domain::INVALID_ID};
    ProjectContexts m_projects;
    Scene::BedRenderUpdater m_bed_render_updater;
    Scene::CameraFrustumUpdater m_camera_frustum_updater;
    HoverData m_hover_data;
    Platform::AnimationManager& m_animation_manager;
    Scene::GeometryDataFactory m_data_factory;

    bool m_freeze_selection_center{ false };
    bool m_volume_materials_dirty{ true };
};

} // namespace Slic3r::App::Plater
