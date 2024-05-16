#pragma once

#include <memory>

#include <wx/wx.h>

#include <Slic3r/App/TestRenderModule.hpp>

namespace Slic3r::App::Desktop {
class MainFrame;

class DesktopApp : public wxApp {
public:
    bool OnInit() override;

private:

    MainFrame* m_main_frame;
    TestRenderModule m_render_module;

};

} // namespace Slic3r::App::Desktop

