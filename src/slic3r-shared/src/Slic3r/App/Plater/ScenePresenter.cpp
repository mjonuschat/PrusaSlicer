#include "Slic3r/App/Plater/ScenePresenter.hpp"

namespace Slic3r::App::Plater {

ScenePresenter::ScenePresenter(
    const Domain::Workbench& m_workbench, Biz::ProjectInteractor& project_interactor
)
    : m_workbench(m_workbench), m_project_interactor(project_interactor)
{
//    std::for_each(m_workbench.projects().begin(), m_workbench.projects().end(), [this](const auto& p) {
//        m_projects.emplace(p.first, ScenePresenterProjectContext{});
//    });
    on_selected_project_changed(m_project_interactor.selected_project_id());
}

void ScenePresenter::render_scene(Render::CommandBuffer& command_buffer)
{
    if (!m_projects.empty())
        project_context().scene().render(command_buffer);
}

void ScenePresenter::render_imgui(const Render::ScreenInfo& screen_info)
{
    if (!m_projects.empty())
        project_context().scene().render_imgui(screen_info);
}


void ScenePresenter::update_cameras(const std::function<void(Scene::Camera&)>& modifier)
{
    std::for_each(m_projects.begin(), m_projects.end(), [modifier](auto& p) {
        modifier(p.second.scene().camera());
    });
}

void ScenePresenter::on_selected_project_changed(size_t index)
{
    m_selected_project_id = index;
    if (m_projects.count(m_selected_project_id) == 0)
        m_projects.emplace(m_selected_project_id, ScenePresenterProjectContext{});
}

void ScenePresenter::on_scene_selection_changed(Domain::SelectionId project_id, const Biz::Scene::Selection& selection)
{

}

void ScenePresenter::on_instance_added(Domain::SelectionId project_id, const Domain::ElementRefs& instances)
{
    // if first instance, create geometry
    // create nodes for all volumes under instance

}

void ScenePresenter::on_instance_removed(Domain::SelectionId project_id, const Domain::ElementRefs& instances)
{

}

void ScenePresenter::on_instance_transformed(Domain::SelectionId project_id, const Domain::ElementRefs& elements)
{

}


void ScenePresenter::on_volume_added(Domain::SelectionId project_id, const Domain::ElementRefs& volumes)
{

}

void ScenePresenter::on_volume_removed(Domain::SelectionId project_id, const Domain::ElementRefs& volumes)
{

}

void ScenePresenter::on_volume_transformed(Domain::SelectionId project_id, const Domain::ElementRefs& elements)
{

}

void ScenePresenter::on_volume_mesh_changed(Domain::SelectionId project_id, const Domain::ElementRefs& volumes)
{

}


void ScenePresenter::on_bed_added(Domain::SelectionId project_id, size_t idx)
{

}

void ScenePresenter::on_bed_removed(Domain::SelectionId project_id, size_t idx)
{

}

void ScenePresenter::on_bed_transformed(Domain::SelectionId project_id, size_t idx)
{

}


void ScenePresenter::on_wipe_tower_added(Domain::SelectionId project_id, size_t idx)
{

}

void ScenePresenter::on_wipe_tower_removed(Domain::SelectionId project_id, size_t idx)
{

}

void ScenePresenter::on_wipe_tower_transformed(Domain::SelectionId project_id, size_t idx)
{

}



} // namespace Slic3r::App::Plater
