#include "ProjectInteractor.hpp"

#include <Slic3r/Domain/Workbench.hpp>
#include <Slic3r/Domain/Project.hpp>
#include <Slic3r/Domain/Bed.hpp>
#include <libslic3r/Model.hpp>

#include "ISelectedProjectChangedListener.hpp"
#include "IProjectsChangedListener.hpp"

namespace Slic3r::Biz {

SelectionId ProjectInteractor::new_project()
{
    Domain::Project project;
    initialize_new_project(project);
    SelectionId container_id = project.config_containers().front()->id().id;
    SelectionId project_id = add_project(std::move(project));
    do_select_project(project_id);
    do_select_config_container(container_id);
    return project_id;
}

void ProjectInteractor::initialize_new_project(Domain::Project& p)
{
    auto& cc_ptr = p.config_containers().emplace_back(std::make_unique<Domain::ConfigContainer>());
}

void ProjectInteractor::select_project(SelectionId project_id)
{
    if (project_id != m_selection.project_id)
        return;
    do_select_project(project_id);
}

void ProjectInteractor::do_select_project(SelectionId project_id)
{
    m_selection.project_id = project_id;
    m_selected_project_changed_listeners.invoke([project_id](auto* l) {
        l->on_project_changed(project_id);
    });
}

void ProjectInteractor::do_select_config_container(SelectionId container_id)
{
    m_selection.config_container_id = container_id;
    SelectionId project_id = m_selection.project_id;
    m_selected_config_container_changed_listener.invoke([container_id, project_id](auto* l) {
        l->on_selected_config_container_changed(project_id, container_id);
    });
}

SelectionId ProjectInteractor::add_project(Domain::Project&& p)
{
    auto& projects = m_workbench.projects();
    const SelectionId first_container_id = p.config_containers().front()->id().id;
    projects.emplace_back(std::move(p));
    SelectionId project_id = projects.size() - 1;
    m_projects_changed_listeners.invoke([project_id](auto* l) { l->on_project_added(project_id); });
    do_select_project(project_id);
    do_select_config_container(first_container_id);
    return project_id;
}

void ProjectInteractor::remove_project(SelectionId project_id)
{
    m_projects_changed_listeners.invoke([project_id](auto* l) { l->on_project_removed(project_id); });

    auto& projects = m_workbench.projects();
    auto it = projects.begin();
    std::advance(it, project_id);
    projects.erase(it);

    if (m_selection.project_id >= project_id) {
        if (m_selection.project_id >= m_workbench.projects().size()) {
            m_selection.project_id--;
        }
        do_select_project(m_selection.project_id);
    }
}

} // namespace Slic3r::Biz
