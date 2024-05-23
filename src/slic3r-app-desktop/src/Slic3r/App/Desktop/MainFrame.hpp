#pragma once

#include <memory>

#include <wx/wx.h>
#include "Slic3r/App/Platform/WX/WXRenderCanvas.hpp"
#include "TopBarMenus.hpp"

namespace Slic3r::App::Desktop {

class TopBar;

class MainFrame : public wxFrame {
public:
    MainFrame();

    Platform::WX::WXRenderCanvas& get_render_canvas() { return *m_canvas; }

private:
    std::unique_ptr<Platform::WX::WXRenderCanvas> m_canvas;

    TopBarMenus         m_top_bar_menus;
    TopBar*             m_top_bar{ nullptr };

    // Move to BasicAppConfig 
    /*ConfigOptionMode*/ int m_mode{ 1 /*comAdvanced*/ };
};

}