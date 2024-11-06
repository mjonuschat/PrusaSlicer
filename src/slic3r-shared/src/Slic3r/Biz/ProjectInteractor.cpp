#include "Slic3r/Biz/ProjectInteractor.hpp"

#include <Slic3r/Domain/Workbench.hpp>
#include <Slic3r/Domain/Project.hpp>
#include <Slic3r/Domain/Bed.hpp>
#include <libslic3r/Model.hpp>

#include "Slic3r/Biz/ISelectedProjectChangedListener.hpp"
#include "Slic3r/Biz/IProjectsChangedListener.hpp"

namespace Slic3r::Biz {

Domain::SelectionId ProjectInteractor::new_project()
{
    Domain::Project project;
    initialize_new_project(project);
    Domain::SelectionId container_id = project.config_containers().front()->id().id;
    Domain::SelectionId project_id = add_project(std::move(project));
    return project_id;
}

void ProjectInteractor::initialize_new_project(Domain::Project& p)
{
    auto& cc_ptr = p.config_containers().emplace_back(std::make_unique<Domain::ConfigContainer>());
    // upload config from selected preset
    cc_ptr->set_print_config(m_workbench.preset_bundle().full_config());
}

void ProjectInteractor::select_project(Domain::SelectionId project_id)
{
    if (project_id != m_selection.project_id)
        do_select_project(project_id);
}

void ProjectInteractor::do_select_project(Domain::SelectionId project_id)
{
    m_selection.project_id = project_id;
    m_selected_project_changed_listeners.invoke([project_id](auto* l) {
        l->on_selected_project_changed(project_id);
    });
}

void ProjectInteractor::do_select_config_container(Domain::SelectionId container_id)
{
    m_selection.config_container_id = container_id;
    Domain::SelectionId project_id = m_selection.project_id;
    m_selected_config_container_changed_listener.invoke([container_id, project_id](auto* l) {
        l->on_selected_config_container_changed(project_id, container_id);
    });
}

Domain::SelectionId ProjectInteractor::add_project(Domain::Project&& p)
{
    auto& projects = m_workbench.projects();
    const Domain::SelectionId first_container_id = p.config_containers().front()->id().id;
    Domain::SelectionId project_id = m_workbench.next_project_id();
    projects.emplace(project_id, std::move(p));
    m_projects_changed_listeners.invoke([project_id](auto* l) { l->on_project_added(project_id); });
    do_select_project(project_id);
    do_select_config_container(first_container_id);
    return project_id;
}

void ProjectInteractor::remove_project(Domain::SelectionId project_id)
{
    m_projects_changed_listeners.invoke([project_id](auto* l) { l->on_project_removed(project_id); });

    auto& projects = m_workbench.projects();
    auto it = projects.find(project_id);

    it = projects.erase(it);

    if (m_selection.project_id == project_id) {
        Domain::SelectionId next_selected_project_id = Domain::INVALID_ID;
        if (it != projects.end())
            next_selected_project_id = it->first;
        do_select_project(next_selected_project_id);
    }
}

} // namespace Slic3r::Biz
