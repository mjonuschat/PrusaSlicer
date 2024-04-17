#pragma once

#include <memory>

#include <wx/wx.h>
#include "Slic3r/App/Platform/WX/WXRenderCanvas.hpp"

namespace Slic3r::App::WXTest {

class MainFrame : public wxFrame {
public:
    MainFrame();

    Platform::WX::WXRenderCanvas& get_render_canvas() { return *m_canvas; }

private:
    std::unique_ptr<Platform::WX::WXRenderCanvas> m_canvas;
};

}