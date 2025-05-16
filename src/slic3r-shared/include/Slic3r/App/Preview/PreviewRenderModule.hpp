#pragma once

#include "Slic3r/App/Platform/AbstractRenderModule.hpp"
#include "Slic3r/App/Preview/PreviewScenePresenter.hpp"
#include "Slic3r/App/Scene/GizmoManager.hpp"
#include "Slic3r/Biz/Preset/PresetInteractor.hpp"
#include "Slic3r/App/Preview/PreviewRenderLayout.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/App/Preview/FdmViewerWrapper.hpp"
#include "Slic3r/App/Preview/SlaViewerWrapper.hpp"
#include "Slic3r/App/Preview/SidebarPreviewActionButtons.hpp"
#include "Slic3r/App/Preview/SidebarAutoReslice.hpp"
#include <Slic3r/App/Preview/Legend.hpp>
#include <Slic3r/App/Preview/GCodeWindow.hpp>
#include <Slic3r/App/Preview/DoubleSliderForGCode.hpp>
#include <Slic3r/App/Preview/DoubleSliderForLayers.hpp>

#include <memory>

namespace Slic3r::App::Preview {

struct ExtrudersSequence;
} // namespace Slic3r::App::LibvgcodeWrapper

namespace Slic3r::App::Preview {

class SidebarPreviewActionButtons;

class PreviewRenderModule final : public Platform::AbstractRenderModule,
                                  public Biz::ISelectedBedInstanceChangedListener,
                                  public Biz::IFDMResultCacheChangedListener,
                                  public Biz::IStatusCacheChangedListener
{
public:
    PreviewRenderModule(const Domain::Workbench& workbench, Biz::ProjectInteractor& project_interactor)
        : m_workbench(workbench), m_project_interactor(project_interactor)
    {}

    /**
     * @name Implementation of Platform::AbstractRenderModule public interface
     * @{
     */
    void render_scene(Render::CommandBuffer& cmd_buffer) override;
    void render_imgui(Render::CommandBuffer& cmd_buffer) override;
    void on_scene_mouse_event(const Platform::MouseEvent& e) override;
    void on_scene_keyboard_event(const Platform::KeyboardEvent& e) override;
    void add_type_changed_listener(IRenderModuleChangedListener* l) override;
    void remove_type_changed_listener(IRenderModuleChangedListener* l) override;
    /**@}*/

    void on_selected_bed_instance_changed(
        Domain::SelectionId project_id,
        Domain::SelectionId container_id,
        Domain::SelectionId bed_instance_id
    ) override;

    void on_fdm_result_cache_changed(
        const Biz::Slicing::SlicingId id
    ) override { update_fdm_viewer_data(id); }
/*
    void on_sla_result_cache_changed(
        const Biz::Slicing::SlicingId id
    ) override { update_sla_viewer_data(id); }
*/

    void on_status_cache_changed(
        const Biz::Slicing::SlicingId id
    ) override;

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
    Yoga::Passthrough<ObjectList> m_object_list;
    Yoga::Passthrough<CubeView> m_cube_view;
    Yoga::Passthrough<SidebarBed> m_sidebar_bed;
    Yoga::Passthrough<SidebarPrint> m_sidebar_print;
    Yoga::Passthrough<SidebarPreviewActionButtons> m_sidebar_action_buttons;
    Yoga::Passthrough<GCodeWindow> m_gcode_window;
    Yoga::Passthrough<Legend> m_legend;
    Yoga::Passthrough<DoubleSliderForGcode> m_slider_gcode;
    Yoga::Passthrough<DoubleSliderForLayers> m_slider_layers;
    Yoga::Passthrough<SidebarAutoReslice> m_sidebar_auto_reslice;
    // temporary variable to allow to switch yoga layout on/off

    Yoga::ToolbarButton* m_button_travels = nullptr;
    Yoga::ToolbarButton* m_button_wipes = nullptr;
    Yoga::ToolbarButton* m_button_retractions = nullptr;
    Yoga::ToolbarButton* m_button_unretractions = nullptr;
    Yoga::ToolbarButton* m_button_seams = nullptr;
    Yoga::ToolbarButton* m_button_tool_changes = nullptr;
    Yoga::ToolbarButton* m_button_color_changes = nullptr;
    Yoga::ToolbarButton* m_button_pause_prints = nullptr;
    Yoga::ToolbarButton* m_button_custom_gcodes = nullptr;
    Yoga::ToolbarButton* m_button_center_of_gravity = nullptr;
    Yoga::ToolbarButton* m_button_tool_marker = nullptr;
    Yoga::ToolbarButton* m_button_shells = nullptr;
    Yoga::ToolbarButton* m_button_legend = nullptr;
    Yoga::ToolbarButton* m_button_gcode = nullptr;

    std::unordered_set<IRenderModuleChangedListener*> m_render_module_changed_listeners;

private:
    void init_gizmos();
    void init_viewers(Render::Device& device);
    void update_fdm_viewer_data(const Biz::Slicing::SlicingId id);
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
};

} // namespace Slic3r::App::Preview
