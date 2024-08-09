#pragma once

#include <memory>

#include <wx/wx.h>
#include "Slic3r/App/Platform/WX/WXRenderCanvas.hpp"
#include "Slic3r/Domain/Workbench.hpp"
#include "Slic3r/Domain/Bed.hpp"
#include "TopBarMenus.hpp"

namespace Slic3r::App::Desktop {

class TopBar;

class MainFrame : public wxFrame {
public:
    explicit MainFrame(Domain::Workbench& workbench);

    Platform::WX::WXRenderCanvas& get_render_canvas() { return *m_canvas; }

private:
    Domain::Workbench& m_workbench;
    std::unique_ptr<Platform::WX::WXRenderCanvas> m_canvas;

    TopBarMenus         m_top_bar_menus;
    TopBar*             m_top_bar{ nullptr };

    // Move to BasicAppConfig 
    /*ConfigOptionMode*/ int m_mode{ 1 /*comAdvanced*/ };
};

}