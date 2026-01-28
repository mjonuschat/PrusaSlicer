#pragma once

#include <map>

#include <wx/wx.h>
#include "Slic3r/App/Platform/WX/WXRenderCanvas.hpp"
#include "Slic3r/App/ILanguageChangedListener.hpp"
#include "Slic3r/Domain/Workbench.hpp"
#include "Slic3r/Domain/Bed.hpp"
#include "Slic3r/Biz/Preset/PresetInteractor.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/App/Desktop/TabsBarMenus.hpp"
#include "Slic3r/App/LeftBarTabs.hpp"
#include "Slic3r/App/IAppConfigChangedListener.hpp"

namespace Slic3r::App::Desktop::Preset {
class AbstractEditor;
} // namespace Slic3r::App::Desktop::Preset

namespace Slic3r::Biz {
class ProjectInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::App {
class Navigator;
} // namespace Slic3r::App

namespace Slic3r::App::Desktop {

#ifdef WIN32
constexpr int WM_USER_MEDIACHANGED{0x7FFF}; // WM_USER from 0x0400 to 0x7FFF, picking the last one to not interfere with wxWidgets allocation
#endif // WIN32

class LeftBar;

class MainFrame : public wxFrame, public ILanguageChangedListener, public IAppConfigChangedListener
{
public:
    MainFrame(
        Domain::Workbench& workbench,
        Biz::ProjectInteractor& project_interactor,
        Navigator& navigator
    );
    ~MainFrame();

    Platform::WX::WXRenderCanvas& get_render_canvas()
    {
        return *m_canvas;
    }

    void sys_color_changed();
    bool select_language();

    // set language, font and all other ui settings for canvas
    void update_canvas_ui_settings();
    void update_graphics_settings();

#ifdef WIN32
    // Register Win32 RawInput callbacks (3DConnexion) and removable media insert / remove callbacks.
    // Called from wxEVT_ACTIVATE, as wxEVT_CREATE was not reliable (bug in wxWidgets?).
    void register_win32_callbacks();
#endif // WIN32

    void update_left_bar();

    void switch_left_tab(LeftBarTabs id, const std::string& data);

    void update_accel_table();

private:
    void init_left_bar(Biz::ProjectInteractor& project_interactor);
    void init_printer_page(Biz::ProjectInteractor& project_interactor);
    void init_projects_page();
    void init_slicing_page();
    void init_printables_page(Biz::ProjectInteractor& project_interactor);
    void init_preferences_button();
    void complete_and_bind_left_bar();

    void on_language_changed() override;
    void on_app_config_changed() override;

    void on_close(wxCloseEvent& event);

    // @note next frame related functions use wxTopLevelWindow* window
    // because they can be called for settings non-modal dialog in the feature
    void window_pos_save(wxTopLevelWindow* window, const std::string& name);
    void window_pos_restore(
        wxTopLevelWindow* window,
        const std::string& name,
        bool default_maximized = false
    );
    void window_pos_sanitize(wxTopLevelWindow* window);
    void persist_window_geometry(wxTopLevelWindow* window, bool default_maximized = false);

private:
    Domain::Workbench& m_workbench;
    Biz::ProjectInteractor& m_project_interactor;
    Biz::Preset::PresetInteractor& m_preset_interactor;
    std::unique_ptr<Platform::WX::WXRenderCanvas> m_canvas;
    Navigator& m_navigator;

    TabsBarMenus m_tabs_bar_menus;
    LeftBar* m_left_bar{nullptr};

    wxAcceleratorTable m_accel_table;

#ifdef WIN32
    void* m_hDeviceNotify{nullptr};
    uint32_t m_ulSHChangeNotifyRegister{0};
#endif // WIN32

    bool m_printables_page_added {false};
    bool m_printers_page_added {false};
};

} // namespace Slic3r::App::Desktop
