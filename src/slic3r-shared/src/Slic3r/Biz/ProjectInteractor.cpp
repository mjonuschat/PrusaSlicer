#include "Slic3r/Biz/ProjectInteractor.hpp"

#include <Slic3r/Domain/Workbench.hpp>
#include <Slic3r/Domain/Project.hpp>
#include <Slic3r/Domain/Bed.hpp>
#include <libslic3r/Model.hpp>

#include "Slic3r/Biz/ISelectedProjectChangedListener.hpp"
#include "Slic3r/Biz/IProjectsChangedListener.hpp"
#include "Slic3r/Biz/ISelectedBedInstanceChangedListener.hpp"

namespace Slic3r::Biz {

Domain::SelectionId ProjectInteractor::new_project()
{
    Domain::Project project;
    initialize_new_project_before_inserting(project);
    Domain::SelectionId project_id = add_project(std::move(project));
    return project_id;
}

void ProjectInteractor::initialize_new_project_before_inserting(Domain::Project& p)
{
    auto& cc_ptr = p.config_containers().emplace_back(std::make_unique<Domain::ConfigContainer>());
    // upload config from selected preset
    cc_ptr->set_print_config(m_workbench.preset_bundle().full_config());
}

void ProjectInteractor::initialize_inserted_project(size_t project_id)
{
    auto& p = m_workbench.project(project_id);
    const auto& cc_ptr = m_workbench.project(project_id).config_containers().front();
    size_t cc_id = cc_ptr->id().id;
    const auto& selected_printer_preset =
        m_preset_interactor.config_container_context(project_id, cc_id).printer.edited_preset;

    Domain::Bed& bed = p.bed_container().add_bed(selected_printer_preset, m_workbench.preset_bundle());
    cc_ptr->set_bed(bed);
    m_scene_interactor.add_bed_instance(cc_id);

    p.update_instances_bed_placement();
}

void ProjectInteractor::select_project(Domain::SelectionId project_id)
{
    if (project_id != m_selection.project_id)
        do_select_project(project_id);
}

void ProjectInteractor::on_selected_bed_instance_changed(Domain::SelectionId project_id, Domain::SelectionId container_id, Domain::SelectionId bed_instance_id)
{
    if (container_id != m_selection.config_container_id)
        do_select_config_container(container_id);
}

void ProjectInteractor::update_bed(const Domain::SelectionId bed_instance_id) {
    const Domain::ConfigContainer& config_container{selected_config_container()};
    Model &model{m_workbench.project(selected_project_id()).model()};
    const Domain::ConfigContainer::BedInstanceList& instances{ config_container.bed_instances()};

    const auto instance{std::ranges::find_if(
        instances,
        [&](const std::unique_ptr<Slic3r::Domain::BedInstance>& instance) {
            return instance->id().id == bed_instance_id;
        }
    )};
    ASSERT(instance != instances.end());

    m_slicing_interactor.update_process(
        model,
        config_container.print_config(),
        **instance
    );
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
    const auto& config_container = *p.config_containers().front();
    const Domain::SelectionId first_container_id = config_container.id().id;
    Domain::SelectionId project_id = m_workbench.next_project_id();
    projects.emplace(project_id, std::move(p));
    m_projects_changed_listeners.invoke([project_id](auto* l) { l->on_project_added(project_id); });
    do_select_project(project_id);
    do_select_config_container(first_container_id);

    initialize_inserted_project(project_id);

    const Domain::SelectionId first_bed_instance_id = config_container.bed_instances().front()->id().id;
    m_scene_interactor.select_bed_instance({ first_container_id, first_bed_instance_id });

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
