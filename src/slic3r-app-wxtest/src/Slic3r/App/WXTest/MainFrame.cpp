#include "MainFrame.hpp"

namespace Slic3r::App::WXTest {
MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, "")
{
    m_canvas = std::make_unique<Platform::WX::WXRenderCanvas>(this);
}

}
