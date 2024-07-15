#include "DesktopApp.hpp"
#include "MainFrame.hpp"

#include <Slic3r/Log.hpp>
#include <Slic3r/App/WX/WidgetsConfig.hpp>
#include <Slic3r/App/Init.hpp>

#include <Slic3r/App/Platform/PlatformServices.hpp>



wxIMPLEMENT_APP(Slic3r::App::Desktop::DesktopApp);

namespace Slic3r::App::Desktop {
bool DesktopApp::OnInit()
{
    init_logging();
    set_log_level(4);

    init_paths();
    const bool is_dark = true;
    const bool is_sys_menu = true;
    WX::WidgetsConfig* wdts_config = WX::WidgetsConfig::instance(is_dark, is_sys_menu);

    m_main_frame = new MainFrame();
    Platform::WX::WXRenderCanvas& canvas = m_main_frame->get_render_canvas();
    Platform::PlatformServices::instance().set_services(&canvas, &canvas);
    
    canvas.set_render_module(&m_render_module);
    m_main_frame->Show();

    // Initial repaint
    canvas.render();

    return true;
}

}
