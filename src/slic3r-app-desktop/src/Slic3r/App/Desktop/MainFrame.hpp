#pragma once

#include <memory>
#include <map>

#include <wx/wx.h>
#include "Slic3r/App/Platform/WX/WXRenderCanvas.hpp"
#include "Slic3r/Domain/Workbench.hpp"
#include "Slic3r/Domain/Bed.hpp"
#include "Slic3r/Biz/Preset/PresetInteractor.hpp"
#include "Slic3r/Biz/Preset/PresetInteractorConfigContainerContext.hpp"
#include "TopBarMenus.hpp"

namespace Slic3r::App::Desktop::Preset {
class AbstractEditor;
}

namespace Slic3r::App::Desktop {

class TopBar;

class MainFrame : public wxFrame {
public:
    explicit MainFrame(Domain::Workbench& workbench);

    Platform::WX::WXRenderCanvas& get_render_canvas() { return *m_canvas; }

    void    sys_color_changed();

private:
    Domain::Workbench& m_workbench;
    Biz::Preset::PresetInteractor m_preset_interactor;
    std::unique_ptr<Platform::WX::WXRenderCanvas> m_canvas;

    std::map<Slic3r::Preset::Type, Preset::AbstractEditor*>   m_preset_editors;

    TopBarMenus         m_top_bar_menus;
    TopBar*             m_top_bar{ nullptr };

    // Move to BasicAppConfig 
    /*ConfigOptionMode*/ int m_mode{ 1 /*comAdvanced*/ };

    void init_top_bar();
    void init_plater();
    void init_preset_editors();
    void add_preset_editor(Preset::AbstractEditor* panel, const std::string& bmp_name /*= ""*/);

    void complete_and_bind_top_bar();

    Biz::Preset::PresetInteractorConfigContainerContext*             m_active_context{ nullptr };
    // just for test 
    std::vector<Biz::Preset::PresetInteractorConfigContainerContext> m_bed_contexts;
    void init_bed_context();

};

}