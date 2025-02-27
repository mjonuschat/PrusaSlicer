#pragma once

#include "Slic3r/Domain/Workbench.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/ISelectedProjectChangedListener.hpp"
#include "Slic3r/App/Scene/ISceneProvider.hpp"
#include "Slic3r/App/Scene/ScenePresenterProjectContext.hpp"
#include "Slic3r/App/Preview/PreviewAuxiliaryElementId.hpp"
#include "Slic3r/Domain/SelectionId.hpp"
#include "Slic3r/App/Preview/PreviewSceneRenderCustomizer.hpp"

namespace Slic3r::App::Preview {

class PreviewScenePresenter : public Biz::ISelectedProjectChangedListener,
                              public PreviewSceneRenderCustomizer,
                              public Scene::ISceneProvider
{
public:
    using ScenePresenterProjectContext = Scene::ScenePresenterProjectContext<PreviewAuxiliaryElementId>;
    using ProjectContexts = std::unordered_map<Domain::SelectionId, ScenePresenterProjectContext>;

    PreviewScenePresenter(
        const Domain::Workbench& m_workbench,
        Biz::ProjectInteractor& project_interactor,
        Render::Device& device
    );

    void render_scene(Render::CommandBuffer& command_buffer);
    void render_imgui(const Render::ScreenInfo& screen_info);

    void screen_resized(const Render::Rect& viewport);

    /**
     * @name Implementation of Scene::ISceneProvider public interface
     * @{
     */
    virtual Scene::Scene& scene() override { return project_context().scene(); }
    virtual const Scene::Scene& scene() const override { return project_context().scene(); }
    virtual Scene::SceneChangeSession& selection_scene_changes() override {
        return project_context().selection_scene_changes();
    }
    virtual Scene::Node& selection_root() override {
        return project_context().selection_root();
    }
    /**@}*/

    /**
     * @name Implementation of Biz::ISelectedProjectChangedListener public interface
     * @{
     */
    virtual void on_selected_project_changed(size_t index) override;
    /**@}*/

    Scene::ScenePresenterProjectContext<PreviewAuxiliaryElementId>& project_context()
    {
        ASSERT(m_selected_project_id != Domain::INVALID_ID);
        return m_projects[m_selected_project_id];
    }

    const Scene::ScenePresenterProjectContext<PreviewAuxiliaryElementId>& project_context() const
    {
        ASSERT(m_selected_project_id != Domain::INVALID_ID);
        return m_projects.find(m_selected_project_id)->second;
    }

private:
    void update_cameras(const std::function<void(Scene::Camera&)>& modifier);

private:
    const Domain::Workbench& m_workbench;
    Biz::ProjectInteractor& m_project_interactor;
    Render::Device& m_device;

    Domain::SelectionId m_selected_project_id{ Domain::INVALID_ID };
    ProjectContexts m_projects;

};

} // namespace Slic3r::App::Preview
