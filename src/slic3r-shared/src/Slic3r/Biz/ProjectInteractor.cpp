#include "Slic3r/Biz/ProjectInteractor.hpp"

#include <Slic3r/Domain/Workbench.hpp>
#include <Slic3r/Domain/Project.hpp>
#include <Slic3r/Domain/Bed.hpp>
#include "Slic3r/Domain/Model.hpp"

#include "Slic3r/Biz/ISelectedProjectChangedListener.hpp"
#include "Slic3r/Biz/IProjectsChangedListener.hpp"
#include "Slic3r/Biz/ISelectedBedInstanceChangedListener.hpp"
#include "Slic3r/Biz/UserAccount/ConnectUtils.hpp"
#include "Slic3r/Biz/Config/ConfigLegacy.hpp""

#include "Slic3r/Biz/Format/3mf.hpp"
#include "Slic3r/Biz/ProjectLoader.hpp"

namespace Slic3r::Biz {
Domain::SelectionId ProjectInteractor::new_project()
{
    Domain::Project project;

    initialize_new_project_before_inserting(project);
    Domain::SelectionId project_id = add_project(std::move(project));
    return project_id;
}

void ProjectInteractor::load_project(const std::string& file_path)
{
    Biz::load_project(file_path, [&](Domain::Project&& project){
        add_project(std::move(project));
    });
}

void ProjectInteractor::save_project(const std::string& file_path)
{
    store_3mf(file_path, this->selected_project());
}

void ProjectInteractor::initialize_new_project_before_inserting(Domain::Project& p)
{
    auto& cc_ptr = p.config_containers().emplace_back(std::make_unique<Domain::ConfigContainer>());
    // upload config from selected preset
    cc_ptr->set_print_config(m_workbench.preset_bundle_legacy().full_config());

    if (cc_ptr->print_technology() == Domain::PrinterTechnology::SLA)
        cc_ptr->set_print_config_new(Domain::ConfigPackSLA());
    else
        cc_ptr->set_print_config_new(Domain::ConfigPackFDM());
}

void ProjectInteractor::initialize_inserted_project(size_t project_id)
{
    auto& p = m_workbench.project(project_id);
    for (const auto& cc_ptr : m_workbench.project(project_id).config_containers()) {
        size_t cc_id = cc_ptr->id().id;
        const auto& selected_printer_preset =
            m_preset_interactor.config_container_context(project_id, cc_id).printer.edited_preset;

        Domain::Bed& bed =
            p.bed_container().add_bed(selected_printer_preset, m_workbench.preset_bundle_legacy());
        cc_ptr->set_bed(bed);
        m_scene_interactor.add_bed_instance(cc_id);
    }
    m_scene_interactor.notify_listener_on_objects();
}

void ProjectInteractor::select_project(Domain::SelectionId project_id)
{
    if (project_id != m_selection.project_id) {
        do_select_project(project_id);

        auto& projects = m_workbench.projects();
        const auto& config_container = projects.at(project_id).config_containers().front();
        const Domain::SelectionId first_container_id = config_container->id().id;
        do_select_config_container(first_container_id);

        const Domain::SelectionId first_bed_instance_id = config_container->bed_instances().front()->id().id;
        m_scene_interactor.select_bed_instance({ first_container_id, first_bed_instance_id });
    }
}

ObservableProjectList& ProjectInteractor::observable_project_list()
{
    return m_project_list;
}

Biz::Slicing::SlicingId ProjectInteractor::selected_bed_slicing_id() const
{
    return { selected_project_id(), m_scene_interactor.selected_bed_instance().instance_id };
}

void ProjectInteractor::on_selected_bed_instance_changed(Domain::SelectionId project_id, Domain::SelectionId container_id, Domain::SelectionId bed_instance_id)
{
    if (container_id != m_selection.config_container_id)
        do_select_config_container(container_id);
}

void ProjectInteractor::on_slicing_input_changed(const Domain::BedRef& bed_instance)
{
    const Domain::BedInstance* instance{selected_project().find_bed_instance_by_id(bed_instance.instance_id)};
    ASSERT(instance);
    const auto* config_container{selected_project().find_config_container(bed_instance.config_container_id)};
    ASSERT(config_container);

    m_slicing_interactor.update_process(
        selected_project().model(),
        config_container->new_config(),
        *instance
    );
}

void ProjectInteractor::on_slicing_input_removed(const Domain::BedRef& bed_instance)
{
    m_slicing_interactor.remove_bed(bed_instance.instance_id);
}

void ProjectInteractor::do_select_project(Domain::SelectionId project_id)
{
    m_selection.project_id = project_id;

    invoke_listeners<ISelectedProjectChangedListener>([project_id](auto* l) {
        l->on_selected_project_changed(project_id);
    });
}

void ProjectInteractor::do_select_config_container(Domain::SelectionId container_id)
{
    m_selection.config_container_id = container_id;
    Domain::SelectionId project_id = m_selection.project_id;
    invoke_listeners<ISelectedConfigContainerChangedListener>([container_id, project_id](auto* l) {
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
    invoke_listeners<IProjectsChangedListener>([project_id](auto* l) {
        l->on_project_added(project_id);
    });
    do_select_project(project_id);
    do_select_config_container(first_container_id);

    if (!config_container.bed_instances().empty()) {
        m_preset_interactor.prepare_config_container_preset(project_id, config_container.id().id);
    }
    initialize_inserted_project(project_id);
    const Domain::SelectionId first_bed_instance_id = config_container.bed_instances().front()->id().id;
    m_scene_interactor.select_bed_instance({ first_container_id, first_bed_instance_id });

    return project_id;
}

void ProjectInteractor::remove_project(Domain::SelectionId project_id)
{
    auto& projects = m_workbench.projects();
    auto it = projects.find(project_id);

    ASSERT(it != projects.end());

    invoke_listeners<IProjectsChangedListener>([project_id](auto* l) { l->on_project_will_be_removed(project_id); });

    it = projects.erase(it);

    invoke_listeners<IProjectsChangedListener>([project_id](auto* l) { l->on_project_removed(project_id); });

    // At least one project need to exist at all times
    if (projects.empty()) {
        new_project();
    } else {
        if (m_selection.project_id == project_id) {
            Domain::SelectionId next_selected_project_id = Domain::INVALID_ID;
            if (it != projects.end()) {
                next_selected_project_id = it->first;
            } else {
                next_selected_project_id = projects.begin()->first;
            }

            do_select_project(next_selected_project_id);
        }
    }
}

void ProjectInteractor::do_export(const Slicing::SlicingId id, const boost::filesystem::path& dest_path, bool to_removable)
{
    const std::optional<FDMResultRef> fdm_result{ m_fdm_result_cache.get_result(id) };
    if (!fdm_result)
        return;
    m_last_export_path_storage.set_last_export_path(dest_path, to_removable);
    PrintHost::PrintHostConfig config{Domain::PrintHostType::Local,""};
    PrintHost::PrintHostJobData data{fdm_result.value().get().const_gcode(), dest_path, PrintHost::get_export_format_from_extension(dest_path.extension().string())};
    m_print_host_interactor.export_gcode(std::move(config), std::move(data));
}
void ProjectInteractor::do_upload(const Slicing::SlicingId id, const std::string& filename)
{
    const std::optional<FDMResultRef> fdm_result{ m_fdm_result_cache.get_result(id) };
    if (!fdm_result)
        return;
    PrintHost::PrintHostConfig config{Domain::PrintHostType::OctoPrint,""};
    PrintHost::PrintHostJobData data{fdm_result.value().get().const_gcode(), filename, PrintHost::get_export_format_from_extension(boost::filesystem::path(filename).extension().string())};
    m_print_host_interactor.upload_gcode(std::move(config), std::move(data));
}

void ProjectInteractor::do_upload_connect(const Slicing::SlicingId id, const std::string& connect_msg)
{
    const std::optional<FDMResultRef> fdm_result{ m_fdm_result_cache.get_result(id) };
    if (!fdm_result)
        return;

    PrintHost::PrintHostConfig config{Domain::PrintHostType::PrusaConnect, Network::ServiceConfig::instance().connect_url()};
    config.access_token = m_user_account_interactor.access_token();
    std::string filename;
    std::string body_json;
    if(!UserAccount::ConnectUtils::config_from_json(connect_msg, config, filename, body_json)) {
        SPDLOG_ERROR("Upload to Connect has failed - failed to read Connect message.");
        return;
    }
    PrintHost::PrintHostJobData data{fdm_result.value().get().const_gcode(), filename, PrintHost::get_export_format_from_extension(boost::filesystem::path(filename).extension().string())};
    data.request_body_json = std::move(body_json);
    m_print_host_interactor.upload_gcode(std::move(config), std::move(data));
}

std::string ProjectInteractor::get_project_name(Domain::SelectionId project_id) const
{
    auto it = m_workbench.projects().find(project_id);
    ASSERT(it != m_workbench.projects().end());
    if (it != m_workbench.projects().end()) {
        return it->second.file_name();
    }
    return "default_filename";

}


} // namespace Slic3r::Biz
