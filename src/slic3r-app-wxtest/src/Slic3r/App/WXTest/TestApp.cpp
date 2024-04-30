#include "TestApp.hpp"
#include "MainFrame.hpp"
#include <Slic3r/App/Platform/PlatformServices.hpp>

wxIMPLEMENT_APP(Slic3r::App::WXTest::TestApp);

namespace Slic3r::App::WXTest {
bool TestApp::OnInit()
{
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
