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

namespace Slic3r::App::Desktop::Preset {
class AbstractEditor;
} // namespace Slic3r::App::Desktop::Preset

namespace Slic3r::Biz {
class ProjectInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::App::Desktop {

#ifdef WIN32
constexpr int WM_USER_MEDIACHANGED{0x7FFF}; // WM_USER from 0x0400 to 0x7FFF, picking the last one to not interfere with wxWidgets allocation
#endif // WIN32

class LeftBar;

class MainFrame : public wxFrame, public ILanguageChangedListener
{
public:
    MainFrame(Domain::Workbench& workbench, Biz::ProjectInteractor& project_interactor);
    ~MainFrame();

    Platform::WX::WXRenderCanvas& get_render_canvas()
    {
        return *m_canvas;
    }

    void sys_color_changed();
    bool select_language();

    // set language, font and all other ui settings for canvas
    void update_canvas_ui_settings();

#ifdef WIN32
    // Register Win32 RawInput callbacks (3DConnexion) and removable media insert / remove callbacks.
    // Called from wxEVT_ACTIVATE, as wxEVT_CREATE was not reliable (bug in wxWidgets?).
    void register_win32_callbacks();
#endif // WIN32

private:
    // Move to BasicAppConfig
    /*ConfigOptionMode*/ int m_mode{1 /*comAdvanced*/};

#ifdef OLD_CODE
    void init_plater();
    void init_preset_editors();
    void add_preset_editor(Preset::AbstractEditor* panel, const std::string& bmp_name /*= ""*/);
    void update_preset_editors();
#endif

    void init_left_bar(Biz::ProjectInteractor& project_interactor);
    void init_printer_page(Biz::ProjectInteractor& project_interactor);
    void init_projects_page();
    void init_slicing_page();
    void init_printables_page(Biz::ProjectInteractor& project_interactor);
    void complete_and_bind_left_bar();

    void init_top_bar();
    void complete_and_bind_top_bar();

    void on_language_changed() override;

    void on_close(wxCloseEvent& event);

private:
    Domain::Workbench& m_workbench;
    Biz::ProjectInteractor& m_project_interactor;
    Biz::Preset::PresetInteractor& m_preset_interactor;
    std::unique_ptr<Platform::WX::WXRenderCanvas> m_canvas;

#ifdef OLD_CODE
    std::map<Slic3r::Preset::Type, Preset::AbstractEditor*> m_preset_editors;
#endif

    TabsBarMenus m_tabs_bar_menus;
    LeftBar* m_left_bar{nullptr};

#ifdef WIN32
    void* m_hDeviceNotify{nullptr};
    uint32_t m_ulSHChangeNotifyRegister{0};
#endif // WIN32
};

} // namespace Slic3r::App::Desktop
