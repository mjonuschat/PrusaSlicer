#pragma once

#include <memory>

#include <wx/wx.h>

#include <Slic3r/Domain/Workbench.hpp>
#include <Slic3r/Domain/Bed.hpp>
#include <Slic3r/App/TestRenderModule.hpp>

namespace Slic3r::App::Desktop {
class MainFrame;

class DesktopApp : public wxApp {
public:
    bool OnInit() override;

private:

    MainFrame* m_main_frame;
    TestRenderModule m_render_module;
    Domain::Workbench m_workbench;
};

} // namespace Slic3r::App::Desktop

