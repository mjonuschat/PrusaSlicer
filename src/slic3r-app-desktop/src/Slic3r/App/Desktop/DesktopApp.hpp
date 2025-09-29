#pragma once

#include "Slic3r/App/Platform/WX/WXRenderCanvas.hpp"
#include <memory>

#include <wx/wx.h>

#include <Slic3r/Domain/Workbench.hpp>
#include <Slic3r/Domain/Bed.hpp>
#include <Slic3r/App/Plater/PlaterRenderModule.hpp>
#include <Slic3r/App/Preview/PreviewRenderModule.hpp>
#include <Slic3r/App/Init.hpp>
#include <Slic3r/App/PresetUpdaterUI.hpp>

#include <Slic3r/Biz/ProjectInteractor.hpp>
#include "Slic3r/App/Navigator.hpp"
#include "Slic3r/App/AppConfig.hpp"

namespace Slic3r::App::Desktop {
class MainFrame;

int run(const InitParams& init_params);

class DesktopApp : public wxApp
{
public:
    bool OnInit() override;

    void set_init_params(const InitParams& init_params)
    {
        m_init_params = init_params;
    }

    // TODO: Any recoverable exception should be handled here.
    bool OnExceptionInMainLoop() override;

    void OnUnhandledException() override;

    /**
     * @brief On Windows, accepting message from other instance must be done in wxApp implementation. See register_win32_device_notification_event()
     */
    void handle_app_instance_message(const std::string& message);

    void handle_HID_device_detached_event(const std::string& message);
    void handle_HID_device_attached_event(const std::string& message);
    void handle_volumes_changed_event();

#ifdef __APPLE__
    void MacOpenURL(const wxString& url) override;
#endif /* __APPLE */

private:
    void init_translations();

    std::unique_ptr<AppConfig> m_appconfig;

    std::unique_ptr<wxGLContext> m_gl_context; // do NOT change order of this attribute
    std::unique_ptr<Biz::ProjectInteractor> m_project_interactor;
    std::unique_ptr<PresetUpdaterUI> m_preset_updater_ui;

    MainFrame* m_main_frame{nullptr};
    std::unique_ptr<Plater::PlaterRenderModule> m_plater_module;
    std::unique_ptr<Preview::PreviewRenderModule> m_preview_module;
    Domain::Workbench m_workbench;
    InitParams m_init_params;
    Navigator m_navigator;
};

} // namespace Slic3r::App::Desktop
