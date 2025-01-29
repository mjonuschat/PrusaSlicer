#pragma once

#include <memory>

#include <wx/wx.h>

#include <Slic3r/Domain/Workbench.hpp>
#include <Slic3r/Domain/Bed.hpp>
#include <Slic3r/App/Plater/PlaterRenderModule.hpp>
#include <Slic3r/App/Preview/PreviewRenderModule.hpp>

#include <Slic3r/Biz/ProjectInteractor.hpp>

namespace Slic3r::App::Desktop {
class MainFrame;

class DesktopApp : public wxApp {
public:
    bool OnInit() override;

private:
    void init_translations();

private:

    MainFrame* m_main_frame;
    std::unique_ptr<Plater::PlaterRenderModule> m_plater_module;
    std::unique_ptr<Preview::PreviewRenderModule> m_preview_module;
    Domain::Workbench m_workbench;
    std::unique_ptr<Biz::ProjectInteractor> m_project_interactor;

};

} // namespace Slic3r::App::Desktop

