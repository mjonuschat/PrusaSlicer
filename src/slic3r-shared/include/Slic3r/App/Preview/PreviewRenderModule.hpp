#pragma once

#include "Slic3r/App/Plater/ThumbnailImageGenerator.hpp"
#include "Slic3r/App/Platform/AbstractRenderModule.hpp"
#include "Slic3r/App/Preview/PreviewScenePresenter.hpp"
#include "Slic3r/App/Scene/GizmoManager.hpp"
#include "Slic3r/App/Preview/PreviewRenderLayout.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/App/Preview/FdmViewerWrapper.hpp"
#include "Slic3r/App/Preview/SlaViewerWrapper.hpp"
#include "Slic3r/App/Preview/SidebarPreviewActionButtons.hpp"
#include "Slic3r/App/Preview/SidebarAutoReslice.hpp"
#include <Slic3r/App/Preview/LegendWindow.hpp>
#include <Slic3r/App/Preview/GCodeWindow.hpp>
#include <Slic3r/App/Preview/DoubleSliderForGCode.hpp>
#include <Slic3r/App/Preview/DoubleSliderForLayers.hpp>
#include "Slic3r/Biz/ISelectedProjectChangedListener.hpp"

#include <memory>

namespace Slic3r::App {
struct ThumbnailStore;
class ThumbnailStoreUpdater;
class Navigator;
} // namespace Slic3r::App

namespace Slic3r::App::Preview {

struct ExtrudersSequence;
class SidebarPreviewActionButtons;

class PreviewRenderModule final :
    public Platform::AbstractRenderModule,
    public Biz::ISelectedBedInstancesChangedListener,
    public Biz::IFDMResultCacheChangedListener,
    public Biz::ISelectedProjectChangedListener,
    public Biz::ISLAResultCacheChangedListener,
    public Biz::ISLAObjectCacheChangedListener,
    public Biz::IStatusCacheChangedListener
{
public:
    PreviewRenderModule(
        const Domain::Workbench& workbench,
        Biz::ProjectInteractor& project_interactor,
        std::shared_ptr<ThumbnailStore> thumbnail_store,
        std::shared_ptr<ThumbnailStoreUpdater> thumbnail_store_updater,
        std::shared_ptr<Plater::ThumbnailImageGenerator> thumbnail_image_generator
    ) :
        m_workbench(workbench),
        m_project_interactor(project_interactor),
        m_thumbnail_store(thumbnail_store),
        m_thumbnail_store_updater(thumbnail_store_updater),
        m_thumbnail_image_generator(thumbnail_image_generator)
    {}

    /**
     * @name Implementation of Platform::AbstractRenderModule public interface
     * @{
     */
    void render_scene(Render::CommandBuffer& cmd_buffer) override;
    void render_imgui(Render::CommandBuffer& cmd_buffer) override;
    void on_scene_mouse_event(const Platform::MouseEvent& e) override;
    void on_scene_keyboard_event(const Platform::KeyboardEvent& e) override;
    void set_navigator(Navigator* navigator) override;
    /**@}*/

    void on_selected_bed_instances_changed(Domain::SelectionId project_id, const Biz::Scene::BedSelection& selection) override;

    void on_fdm_result_cache_changed(const Domain::SlicingId id) override
    {
        update_fdm_viewer_data(id);
    }

    void on_sla_result_cache_changed(const Domain::SlicingId& id) override
    {
        update_sla_viewer_result_data(id);
    }

    void on_sla_object_cache_changed(const Domain::SlicingId& id, Domain::ObjectID instance_id) override
    {
        update_sla_viewer_object_data(id, instance_id);
    }

    void on_status_cache_changed(const Domain::SlicingId id) override;

    /**
     * @name Implementation of Biz::ISelectedProjectChangedListener public interface
     * @{
     */
    void on_selected_project_changed(size_t index) override;
    /**@}*/

    void set_sidebars_visible(bool hide) override;

protected:
    /**
     * @name Implementation of Platform::AbstractRenderModule protected interface
     * @{
     */
    void on_init(Render::Device& device, Render::ImguiRender& imgui_render) override;
    void on_activated() override;
    void on_deactivated() override;
    void on_screen_resized() override;
    void register_commands() override;
    /**@}*/

private:
    const Domain::Workbench& m_workbench;
    Biz::ProjectInteractor& m_project_interactor;
    std::unique_ptr<PreviewScenePresenter> m_scene_presenter;
    std::unique_ptr<Scene::GizmoManager> m_gizmo_manager;
    bool m_use_yoga_layout = true;

    FdmViewerWrapper m_fdm_viewer;
    SlaViewerWrapper m_sla_viewer;

    AbstractViewerWrapper* m_viewer = nullptr;

    // main window layout
    std::unique_ptr<PreviewRenderLayout> m_layout;
    // Layout objects
    Yoga::Passthrough<TopBar> m_top_bar;
    Yoga::Passthrough<ObjectListWindow> m_object_list;
    Yoga::Passthrough<CubeView> m_cube_view;
    Yoga::Passthrough<PopNotification::PopNotificationListView> m_pop_notification_list_view;
    Yoga::Passthrough<SidebarBed> m_sidebar_bed;
    Yoga::Passthrough<SidebarPrint> m_sidebar_print;
    Yoga::Passthrough<SidebarPreviewActionButtons> m_sidebar_action_buttons;
    Yoga::Passthrough<GCodeWindow> m_gcode_window;
    Yoga::Passthrough<LegendWindow> m_legend;
    Yoga::Passthrough<DoubleSliderForGcode> m_slider_gcode;
    Yoga::Passthrough<DoubleSliderForLayers> m_slider_layers;
    Yoga::Passthrough<DoubleSliderForLayers> m_sla_slider_layers;
    Yoga::Passthrough<SidebarAutoReslice> m_sidebar_auto_reslice;
    // temporary variable to allow to switch yoga layout on/off

    Yoga::ToolbarButton* m_button_travels           = nullptr;
    Yoga::ToolbarButton* m_button_wipes             = nullptr;
    Yoga::ToolbarButton* m_button_retractions       = nullptr;
    Yoga::ToolbarButton* m_button_unretractions     = nullptr;
    Yoga::ToolbarButton* m_button_seams             = nullptr;
    Yoga::ToolbarButton* m_button_tool_changes      = nullptr;
    Yoga::ToolbarButton* m_button_color_changes     = nullptr;
    Yoga::ToolbarButton* m_button_pause_prints      = nullptr;
    Yoga::ToolbarButton* m_button_custom_gcodes     = nullptr;
    Yoga::ToolbarButton* m_button_center_of_gravity = nullptr;
    Yoga::ToolbarButton* m_button_tool_marker       = nullptr;
    Yoga::ToolbarButton* m_button_shells            = nullptr;
    Yoga::ToolbarButton* m_button_legend            = nullptr;
    Yoga::ToolbarButton* m_button_gcode             = nullptr;

    std::shared_ptr<ThumbnailStore> m_thumbnail_store;
    std::shared_ptr<ThumbnailStoreUpdater> m_thumbnail_store_updater;
    std::shared_ptr<Plater::ThumbnailImageGenerator> m_thumbnail_image_generator;

    Navigator* m_render_module_navigator{nullptr};

private:
    void init_gizmos();
    void init_viewers(Render::Device& device);
    void update_fdm_viewer_data(const Domain::SlicingId id);
    void update_sla_viewer_result_data(const Domain::SlicingId id);
    void update_sla_viewer_object_data(const Domain::SlicingId id, Domain::ObjectID instance_id);
    void update_sla_viewer_data(const Domain::SlicingId id);
    void init_scene_layout();
    void update_toolbar_visibility();

    void on_invalidate_slice();
    void on_update_layers_slider(const Domain::CustomGCode::Info& info);
    void on_request_extra_frames(unsigned int count = 1);
    void on_gcode_view_type_changed();
    void on_slider_layers_on_thumb_move();
    void on_slider_layers_ticks_changed();
    bool on_slider_layers_auto_color_change();
    void on_slider_layers_notify_empty_auto_color_change();
    void on_slider_layers_notify_empty_color_change_gcode();
    bool on_slider_layers_get_extruders_sequence(ExtrudersSequence& sequence);
    int on_slider_layers_show_info_msg(const std::string& message, int btns_flag);
    std::set<int> on_slider_layers_get_used_extruders_in_print(float print_z);
    void on_slider_layers_app_config_changed(const std::string& key, const std::string& val);
    void on_slider_gcode_on_thumb_move();
    void on_legend_shells_action(bool visible);

    void center_camera_on_selected_bed();
    void update_bed_instances();
};

} // namespace Slic3r::App::Preview
