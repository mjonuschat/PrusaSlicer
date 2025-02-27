#pragma once

#include "Slic3r/App/Platform/AbstractRenderModule.hpp"
#include "Slic3r/App/Preview/PreviewScenePresenter.hpp"
#include "Slic3r/App/Scene/GizmoManager.hpp"
#include "Slic3r/Biz/Preset/PresetInteractor.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/App/LibvgcodeWrapper/Wrapper.hpp"

#include <memory>

namespace Slic3r::App::LibvgcodeWrapper {
struct ExtrudersSequence;
} // namespace Slic3r::App::LibvgcodeWrapper

namespace Slic3r::App::Preview {

class PreviewRenderModule final : public Platform::AbstractRenderModule
{
public:
    explicit PreviewRenderModule(const Domain::Workbench& workbench, Biz::ProjectInteractor& project_interactor)
        : m_workbench(workbench), m_project_interactor(project_interactor)
    {}

    /**
     * @name Implementation of Platform::AbstractRenderModule public interface
     * @{
     */
    void render_scene() override;
    void render_imgui() override;
    void on_scene_mouse_event(const Platform::MouseEvent& e) override;
    void on_scene_keyboard_event(const Platform::KeyboardEvent& e) override;
    /**@}*/

protected:
    /**
     * @name Implementation of Platform::AbstractRenderModule protected interface
     * @{
     */
    void on_init(Render::Device& device) override;
    void on_activated() override;
    void on_deactivated() override;
    void on_screen_resized() override;
    /**@}*/

private:
    const Domain::Workbench& m_workbench;
    Biz::ProjectInteractor& m_project_interactor;
    std::unique_ptr<PreviewScenePresenter> m_scene_presenter;
    std::unique_ptr<Scene::GizmoManager> m_gizmo_manager;

    LibvgcodeWrapper::Wrapper m_viewer;

private:
    void init_gizmos();
    void init_viewer(Render::Device& device);
    void send_data_to_viewer();

    void on_invalidate_slice();
    void on_update_layers_slider(const Slic3r::CustomGCode::Info& info);
    void on_request_extra_frames(unsigned int count = 1);
    void on_gcode_view_type_changed();
    void on_slider_layers_on_thumb_move();
    void on_slider_layers_ticks_changed();
    std::vector<std::string> on_slider_layers_get_extruder_colors();
    bool on_slider_layers_auto_color_change();
    void on_slider_layers_check_gcode(Slic3r::CustomGCode::Type type);
    bool on_slider_layers_get_extruders_sequence(LibvgcodeWrapper::ExtrudersSequence& sequence);
    std::string on_slider_layers_get_custom_code(const std::string& code_in, float height);
    std::string on_slider_layers_get_pause_print_msg(const std::string& msg_in, float height);
    std::string on_slider_layers_get_new_color(const std::string& color);
    int on_slider_layers_show_info_msg(const std::string& message, int btns_flag);
    std::string on_slider_layers_get_gcode(Slic3r::CustomGCode::Type type);
    std::set<int> on_slider_layers_get_used_extruders_in_print(float print_z);
    void on_slider_layers_app_config_changed(const std::string& key, const std::string& val);
    void on_slider_gcode_on_thumb_move();
    void on_legend_shells_action(bool visible);
};

} // namespace Slic3r::App::Preview
