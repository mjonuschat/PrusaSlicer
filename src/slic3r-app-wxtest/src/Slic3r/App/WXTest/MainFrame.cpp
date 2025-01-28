#include "MainFrame.hpp"

namespace Slic3r::App::WXTest {
MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, {})
{
    m_canvas = std::make_unique<Platform::WX::WXRenderCanvas>(this);
    m_canvas->set_language("en");
    m_canvas->set_font_size(16.0f);
}

}
