#pragma once

#include "Slic3r/App/Platform/AbstractRenderModule.hpp"
#include "Slic3r/App/Plater/ScenePresenter.hpp"
#include "Slic3r/Biz/Preset/PresetInteractor.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/App/LibvgcodeWrapper/Wrapper.hpp"

#include <memory>

namespace Slic3r::App::Preview {

class PreviewRenderModule final : public Platform::AbstractRenderModule
{
public:
    explicit PreviewRenderModule(const Domain::Workbench& workbench, Biz::ProjectInteractor& project_interactor)
        : m_workbench(workbench), m_project_interactor(project_interactor)
    {}

    void render_scene() override;
    void render_imgui() override;

protected:
    void on_init(Render::Device& device) override;

private:
    const Domain::Workbench& m_workbench;
    Biz::ProjectInteractor& m_project_interactor;
    std::unique_ptr<Plater::ScenePresenter> m_scene_presenter;

    App::LibvgcodeWrapper::Wrapper m_viewer;
};

} // namespace Slic3r::App::Preview
