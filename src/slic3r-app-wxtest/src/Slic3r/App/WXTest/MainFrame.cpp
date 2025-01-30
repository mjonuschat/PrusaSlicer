#include "MainFrame.hpp"

namespace Slic3r::App::WXTest {
MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, {})
{
    auto main_thread_dispatcher{std::make_unique<Platform::StdMainThreadDispatcher>()};
    m_canvas = std::make_unique<Platform::WX::WXRenderCanvas>(this, std::move(main_thread_dispatcher));
    m_canvas->set_language("en");
    m_canvas->set_font_size(16.0f);
}

}
