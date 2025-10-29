#include "Slic3r/Biz/ProjectInteractor.hpp"

#include <Slic3r/Domain/Workbench.hpp>
#include <Slic3r/Domain/Project.hpp>
#include <Slic3r/Domain/Bed.hpp>
#include "Slic3r/Domain/Model.hpp"

#include "Slic3r/Biz/ISelectedProjectChangedListener.hpp"
#include "Slic3r/Biz/IProjectsChangedListener.hpp"
#include "Slic3r/Biz/ISelectedBedInstanceChangedListener.hpp"
#include "Slic3r/Biz/UserAccount/ConnectUtils.hpp"

#include "Slic3r/Biz/Platform/JobManager/JobManager.hpp"
#include "Slic3r/Biz/FileLoadingLogic.hpp"
#include "Slic3r/Biz/Scene/BedFactory.hpp"

#include "Slic3r/Directories.hpp"

#include <boost/filesystem/path.hpp>

namespace Slic3r::Biz {

void ProjectInteractor::initialize_bed(Domain::ConfigContainer& config_container, Domain::BedContainer& bed_container)
{
    const auto& selected_printer_preset{config_container.selected_preset()};
    Domain::Bed& bed{Scene::get_or_create_bed(bed_container, selected_printer_preset, resources_dir())};
    config_container.set_bed(bed);
    m_scene_interactor.add_bed_instance(config_container.id().id);
}

Domain::SelectionId ProjectInteractor::new_project()
{
    Domain::Project project;

    project.config_containers().emplace_back(std::make_unique<Domain::ConfigContainer>());
    auto& config_container = *project.config_containers().front();

    Domain::SelectionId project_id = add_project(std::move(project));

    Domain::Project& added_project{m_workbench.project(project_id)};

    m_preset_interactor.initialize_config_container(config_container);
    initialize_bed(config_container, added_project.bed_container());
    m_scene_interactor.notify_listener_on_objects();

    invoke_listeners<IProjectsChangedListener>([project_id](auto* l) {
        l->on_project_loaded(project_id);
    });

    return project_id;
}

void ProjectInteractor::load_project(const boost::filesystem::path& file_path)
{
    auto on_result{
        [&](Domain::Project&& project)
        {
            if (project.config_containers().empty())
                return;

            const Domain::SelectionId project_id{add_project(std::move(project))};
            Domain::Project& added_project{m_workbench.project(project_id)};

            for (auto& config_container : added_project.config_containers()) {
                m_preset_interactor.load_selected_preset_from_3mf(project_id, config_container->mutable_selected_preset());
            }

            if (added_project.config_containers().empty()) {
                added_project.config_containers().emplace_back(std::make_unique<Domain::ConfigContainer>());
                m_preset_interactor.initialize_config_container(*added_project.config_containers().back());
                initialize_bed(*added_project.config_containers().back(), added_project.bed_container());
            }
            do_select_config_container(added_project.config_containers().front()->id().id);

            m_scene_interactor.notify_listener_on_objects();
            m_scene_interactor.layout_after_project_load(added_project);

            invoke_listeners<IProjectsChangedListener>([project_id](auto* l) {
                l->on_project_loaded(project_id);
            });
        }
    };

    auto on_error{[&](std::exception_ptr eptr, cpptrace::stacktrace)
    {
        std::string description = "Unknown error";
        try {
            std::rethrow_exception(eptr);
        } catch (Loaded3MFException& e) {
            description = fmt::format("Loading file failed: {}", e.issue.msg);
        } catch (std::exception& e) {
            description = fmt::format("Loading file failed: {}", e.what());
        } catch (...) {
        }
        SPDLOG_ERROR(description);
        invoke_listeners<IProjectsChangedListener>([&description](IProjectsChangedListener* l)
        {
            l->on_project_load_failed(description);
        });
    }};

    Platform::PlatformServices::instance()
        .job_manager()
        .create_job(
            "project_load",
            // TODO: preset_bundle may change, making its copy wouldn't help
            [&preset_bundle = m_workbench.preset_bundle(), dialog_provider = m_dialog_provider](
                Biz::JThread::StopToken stop_token,
                const boost::filesystem::path file_path
            ) -> Domain::Project
            {
                return FileLoadingLogic::load_file_as_project(
                    file_path,
                    preset_bundle,
                    dialog_provider
                );
            },
            file_path
        )
        .on_result(on_result)
        .on_exception(on_error)
        .start();
}

void ProjectInteractor::save_project(const std::string& file_path, const Store3mfParam& params)
{
    auto& selected_project = this->selected_project();
    selected_project.increment_version();
    selected_project.set_file_name(boost::filesystem::path(file_path).stem().string());
    store_3mf(file_path, selected_project, params);

    invoke_listeners<IProjectsChangedListener>(
        [this](auto* l) { l->on_project_changed(selected_project_id()); }
    );
}

void ProjectInteractor::select_project(Domain::SelectionId project_id)
{
    if (project_id != m_selection.project_id) {
        do_select_project(project_id);

        auto& projects               = m_workbench.projects();
        const auto& config_container = projects.at(project_id).config_containers().front();
        const Domain::SelectionId first_container_id = config_container->id().id;
        do_select_config_container(first_container_id);
    }
}

ObservableProjectList& ProjectInteractor::observable_project_list()
{
    return m_project_list;
}

Domain::SlicingId ProjectInteractor::selected_bed_slicing_id() const
{
    return {selected_project_id(), m_scene_interactor.bed_selection().last_selected_bed().instance_id};
}

void ProjectInteractor::on_instance_added(
    Domain::SelectionId project_id,
    const Domain::ElementRefs& instances
)
{
    ASSERT(instances.size());

    Domain::Workbench::ProjectMap& projects    = m_workbench.projects();
    Domain::Workbench::ProjectMap::iterator it = projects.find(project_id);

    ASSERT(it != projects.end());

    if (it->second.file_name().empty()) {
        const boost::filesystem::path filename_path(
            it->second.find_object_by_id(instances.front().object_id)->name
        );
        const std::string stem_name = filename_path.stem().string();

        rename_project(project_id, stem_name);
    }
}

void ProjectInteractor::on_selected_bed_instances_changed(Domain::SelectionId project_id, const Scene::BedSelection& selection)
{
    const Domain::BedRef last_selected_bed{selection.last_selected_bed()};
    const Domain::SelectionId container_id{last_selected_bed.config_container_id};

    if (container_id != m_selection.config_container_id)
        do_select_config_container(container_id);
}

void ProjectInteractor::set_dialog_provider(IMessageDialogProvider* dialog_provider)
{
    m_dialog_provider = dialog_provider;
}

void ProjectInteractor::on_slicing_input_changed(const Domain::BedRef& bed_instance)
{
    auto& project = selected_project();
    const Domain::BedInstance* instance{project.find_bed_instance_by_id(bed_instance.instance_id)};
    ASSERT(instance);
    const auto* config_container{project.find_config_container(bed_instance.config_container_id)};
    ASSERT(config_container);

    const auto& selected_preset = config_container->selected_preset();

    m_slicing_interactor.update_process(
        project.model(),
        project.metadata(),
        selected_preset.metadata(),
        config_container->print_config(),
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

    invoke_listeners<ISelectedProjectChangedListener>(
        [project_id](auto* l) { l->on_selected_project_changed(project_id); }
    );
}

void ProjectInteractor::do_select_config_container(Domain::SelectionId container_id)
{
    m_selection.config_container_id = container_id;
    Domain::SelectionId project_id  = m_selection.project_id;
    invoke_listeners<ISelectedConfigContainerChangedListener>(
        [container_id, project_id](auto* l)
        { l->on_selected_config_container_changed(project_id, container_id); }
    );
}

Domain::SelectionId ProjectInteractor::add_project(Domain::Project&& p)
{
    auto& projects                 = m_workbench.projects();
    Domain::SelectionId project_id = m_workbench.next_project_id();
    projects.emplace(project_id, std::move(p));
    invoke_listeners<IProjectsChangedListener>(
        [project_id](auto* l) { l->on_project_added(project_id); }
    );
    // select project
    do_select_project(project_id);
    return project_id;
}

void ProjectInteractor::remove_project(Domain::SelectionId project_id)
{
    auto& projects = m_workbench.projects();
    auto it        = projects.find(project_id);

    ASSERT(it != projects.end());

    invoke_listeners<IProjectsChangedListener>(
        [project_id](auto* l) { l->on_project_will_be_removed(project_id); }
    );

    it = projects.erase(it);

    invoke_listeners<IProjectsChangedListener>(
        [project_id](auto* l) { l->on_project_removed(project_id); }
    );

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

            select_project(next_selected_project_id);
        }
    }
}

void ProjectInteractor::rename_project(Domain::SelectionId project_id, const std::string& new_name)
{
    Domain::Workbench::ProjectMap& projects    = m_workbench.projects();
    Domain::Workbench::ProjectMap::iterator it = projects.find(project_id);

    ASSERT(it != projects.end());

    it->second.set_file_name(new_name);

    invoke_listeners<IProjectsChangedListener>([project_id](auto* l)
                                               { l->on_project_changed(project_id); });
}

void ProjectInteractor::do_export(const Domain::SlicingId id, const boost::filesystem::path& dest_path)
{
    const std::optional<FDMResultRef> fdm_result{m_fdm_result_cache.get_result(id)};
    if (!fdm_result)
        return;
    bool to_removable = m_removable_drive_service.is_path_on_removable_drive(dest_path);
    m_last_export_path_storage.set_last_export_path(dest_path, to_removable);
    PrintHost::PrintHostConfig config{Domain::PrintHostType::Local, ""};
    PrintHost::PrintHostJobData data{
        fdm_result.value().get().const_gcode(),
        dest_path,
        PrintHost::get_export_format_from_extension(dest_path.extension().string())
    };
    m_print_host_interactor.export_gcode(std::move(config), std::move(data));
}

void ProjectInteractor::do_upload(const Domain::SlicingId id, const std::string& filename)
{
    const std::optional<FDMResultRef> fdm_result{m_fdm_result_cache.get_result(id)};
    if (!fdm_result)
        return;
    PrintHost::PrintHostConfig config{Domain::PrintHostType::OctoPrint, ""};
    PrintHost::PrintHostJobData data{
        fdm_result.value().get().const_gcode(),
        filename,
        PrintHost::get_export_format_from_extension(boost::filesystem::path(filename).extension().string())
    };
    m_print_host_interactor.upload_gcode(std::move(config), std::move(data));
}

void ProjectInteractor::do_upload_connect(const Domain::SlicingId id, const std::string& connect_msg)
{
    const std::optional<FDMResultRef> fdm_result{m_fdm_result_cache.get_result(id)};
    if (!fdm_result)
        return;

    PrintHost::PrintHostConfig
        config{Domain::PrintHostType::PrusaConnect, Network::ServiceConfig::instance().connect_url()};
    config.access_token = m_user_account_interactor.access_token();
    std::string filename;
    std::string body_json;
    if (!UserAccount::ConnectUtils::config_from_json(connect_msg, config, filename, body_json)) {
        SPDLOG_ERROR("Upload to Connect has failed - failed to read Connect message.");
        return;
    }
    PrintHost::PrintHostJobData data{
        fdm_result.value().get().const_gcode(),
        filename,
        PrintHost::get_export_format_from_extension(boost::filesystem::path(filename).extension().string())
    };
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

void ProjectInteractor::load_models_to_project(std::vector<boost::filesystem::path> paths)
{
    const auto& proj            = m_workbench.project(selected_project_id());
    Domain::BedRef selected_bed = scene_interactor().bed_selection().last_selected_bed();
    const Domain::ConfigContainer* cc =
        proj.find_config_container(selected_bed.config_container_id);
    const Domain::BedInstance& inst = cc->find_bed_instance(selected_bed.instance_id);
    int nozzle_dmrs_cnt             = cc->selected_preset().hw_config.tool_count;
    FileLoadingLogic::import_files_and_add_to_scene(
        paths,
        nozzle_dmrs_cnt,
        scene_interactor(),
        cc->bed().center(),
        m_dialog_provider
    );
}

} // namespace Slic3r::Biz
