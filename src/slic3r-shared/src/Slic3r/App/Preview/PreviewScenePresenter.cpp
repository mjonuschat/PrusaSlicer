#include "Slic3r/App/Preview/PreviewScenePresenter.hpp"

namespace Slic3r::App::Preview {

PreviewScenePresenter::PreviewScenePresenter(const Domain::Workbench& workbench,
    Biz::ProjectInteractor& project_interactor, Render::Device& device)
    : m_workbench(workbench), m_project_interactor(project_interactor), m_device(device)
{
    size_t project_id = m_project_interactor.selected_project_id();
    on_selected_project_changed(project_id);
}

void PreviewScenePresenter::render_scene(Render::CommandBuffer& command_buffer)
{
    if (!m_projects.empty())
        project_context().scene().render(command_buffer, this);
}

void PreviewScenePresenter::render_imgui(const Render::ScreenInfo& screen_info)
{
    if (!m_projects.empty())
        project_context().scene().render_imgui(screen_info);
}

void PreviewScenePresenter::screen_resized(const Render::Rect& viewport)
{
    update_cameras([&viewport](auto& cam) { cam.set_viewport(viewport); });
}

void PreviewScenePresenter::on_selected_project_changed(size_t index)
{
    m_selected_project_id = index;
    if (m_projects.count(m_selected_project_id) == 0) {
        ScenePresenterProjectContext context{};
        m_projects.emplace(m_selected_project_id, std::move(context));
    }
}

void PreviewScenePresenter::update_cameras(const std::function<void(Scene::Camera&)>& modifier)
{
    std::for_each(m_projects.begin(), m_projects.end(), [modifier](auto& p) {
        modifier(p.second.scene().camera());
    });
}

} // namespace Slic3r::App::Preview
