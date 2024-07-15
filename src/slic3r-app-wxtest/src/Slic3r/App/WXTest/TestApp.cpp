#include "TestApp.hpp"
#include "MainFrame.hpp"
#include <Slic3r/App/Platform/PlatformServices.hpp>
#include <Slic3r/App/Init.hpp>
#include <Slic3r/Log.hpp>

wxIMPLEMENT_APP(Slic3r::App::WXTest::TestApp);

namespace Slic3r::App::WXTest {
bool TestApp::OnInit()
{
    set_log_level(spdlog::level::debug);
    init_logging();
    init_paths();
    m_main_frame = new MainFrame();
    Platform::WX::WXRenderCanvas& canvas = m_main_frame->get_render_canvas();
    Platform::PlatformServices::instance().set_services(&canvas, &canvas);

    m_render_module = std::make_unique<TestRenderModule>();
    canvas.set_render_module(m_render_module.get());
    m_main_frame->Show();

#ifndef __linux__
    // Initial repaint
    canvas.render();
#endif

    return true;
}

}
