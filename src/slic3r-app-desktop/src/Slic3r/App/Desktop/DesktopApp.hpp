#pragma once

#include <memory>

#include <wx/wx.h>

#include <Slic3r/Domain/Workbench.hpp>
#include <Slic3r/Domain/Bed.hpp>
#include <Slic3r/App/TestRenderModule.hpp>

#include <Slic3r/Biz/ProjectInteractor.hpp>

namespace Slic3r::App::Desktop {
class MainFrame;

class DesktopApp : public wxApp {
public:
    bool OnInit() override;

private:
    MainFrame* m_main_frame;
    TestRenderModule m_render_module;
    Domain::Workbench m_workbench;
    std::unique_ptr<Biz::ProjectInteractor> m_project_interactor;
};

} // namespace Slic3r::App::Desktop

