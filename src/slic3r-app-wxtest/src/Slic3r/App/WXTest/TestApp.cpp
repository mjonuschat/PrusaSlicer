#include "TestApp.hpp"
#include "MainFrame.hpp"

wxIMPLEMENT_APP(Slic3r::App::WXTest::TestApp);

namespace Slic3r::App::WXTest {
bool TestApp::OnInit()
{
    m_main_frame = new MainFrame();
    m_main_frame->get_render_canvas().set_render_module(&m_render_module);
    m_main_frame->Show();

    // Initial repaint
    m_main_frame->get_render_canvas().render();

    return true;
}

}
