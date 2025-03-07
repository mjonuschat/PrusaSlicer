#include "TestApp.hpp"
#include "MainFrame.hpp"
#include <Slic3r/Biz/Platform/PlatformServices.hpp>
#include <Slic3r/App/Init.hpp>
#include <Slic3r/Log.hpp>

wxIMPLEMENT_APP(Slic3r::App::WXTest::TestApp);

namespace Slic3r::App::WXTest {
bool TestApp::OnInit()
{
    set_log_level(4);
    init_logging();
    init_paths();
    auto main_thread_dispatcher{std::make_unique<Platform::StdMainThreadDispatcher>()};
    Biz::Platform::PlatformServices::instance().set_main_thread_dispatcher(
        std::move(main_thread_dispatcher)
    );
    m_main_frame = new MainFrame();
    Platform::WX::WXRenderCanvas& canvas = m_main_frame->get_render_canvas();
    Biz::Platform::PlatformServices::instance().set_render_request_handler(&canvas);

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
