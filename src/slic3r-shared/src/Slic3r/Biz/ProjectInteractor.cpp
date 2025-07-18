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

#include "Slic3r/Directories.hpp"

namespace Slic3r::Biz {

void ProjectInteractor::initialize_bed(
    Domain::ConfigContainer& config_container,
    Domain::BedContainer& bed_container
)
{
    const auto& selected_printer_preset{config_container.selected_preset()};
    Domain::Bed& bed{bed_container.get_or_create_bed(selected_printer_preset, Slic3r::resources_dir())};
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

static Domain::Preset::EvaluatedPrinterPreset evaluated_from_selected(
    const Domain::Preset::SelectedPreset& selected_preset
)
{
    using namespace Domain::Preset;

    EvaluatedPrintPreset print_preset;
    print_preset.preset = selected_preset.print;
    print_preset.preset.runtime_only = true;

    AllToolsEvaluatedToolPrintPresets tools;
    for (const EvaluatedToolPrintPreset::Preset& preset : selected_preset.tools) {
        tools.push_back({EvaluatedToolPrintPreset{EvaluatedToolPrintPreset::Preset{preset}}});
        tools.back().back().preset.runtime_only = true;
    }
    print_preset.tools = std::move(tools);

    AllToolsEvaluatedMaterialPresets materials;
    for (const EvaluatedMaterialPreset::Preset& preset : selected_preset.materials) {
        materials.push_back({EvaluatedMaterialPreset{EvaluatedMaterialPreset::Preset{preset}}});
        materials.back().back().preset.runtime_only = true;
    }
    print_preset.materials = std::move(materials);

    EvaluatedPrinterPreset result;

    result.hw_config = selected_preset.hw_config;
    result.preset    = selected_preset.printer;
    result.prints    = EvaluatedPrintPresets{std::move(print_preset)};
    result.preset.runtime_only = true;

    return result;
}

void ProjectInteractor::load_project(const boost::filesystem::path& file_path)
{
    auto on_result{
        [&](Domain::Project&& project)
        {
            const Domain::SelectionId project_id{add_project(std::move(project))};
            Domain::Project& added_project{m_workbench.project(project_id)};

            Domain::Preset::Bundle& preset_bundle{m_workbench.preset_bundle()};
            for (auto& config_container : added_project.config_containers()) {
                const Domain::Preset::SelectedPreset& selected_preset{
                    config_container->selected_preset()
                };
                auto printer_config_inserted{preset_bundle.printer_configs.insert(
                    {selected_preset.hw_config.id, selected_preset.hw_config}
                ).second};
                auto evaluated_preset_inserted{preset_bundle.evaluated_presets.insert(
                    {selected_preset.hw_config.id, {evaluated_from_selected(selected_preset)}}
                ).second};

                // This requires a matching and diffing mechanism.
                ASSERT(printer_config_inserted && evaluated_preset_inserted, "TODO, this codepath");
            }

            if (added_project.config_containers().empty()) {
                added_project.config_containers().emplace_back(
                    std::make_unique<Domain::ConfigContainer>()
                );
                m_preset_interactor.initialize_config_container(
                    *added_project.config_containers().back()
                );
            }

            for (auto& config_container : added_project.config_containers()) {
                initialize_bed(*config_container, added_project.bed_container());
            }

            m_scene_interactor.notify_listener_on_objects();

            invoke_listeners<IProjectsChangedListener>([project_id](auto* l) {
                l->on_project_loaded(project_id);
            });
        }
    };

    Platform::PlatformServices::instance()
        .job_manager()
        .create_job(
            "project_load",
            [](Biz::JThread::StopToken stop_token,
               const boost::filesystem::path file_path) -> Domain::Project
            { return FileLoadingLogic::load_file_as_project(file_path); },
            file_path
        )
        .on_result(on_result)
        .start();
}

void ProjectInteractor::save_project(const std::string& file_path, const Store3mfParam& params)
{
    auto& selected_project = this->selected_project();
    selected_project.increment_version();
    selected_project.set_file_name(file_path);
    store_3mf(file_path, selected_project, params);

    invoke_listeners<IProjectsChangedListener>([this](auto* l) {
        l->on_project_changed(selected_project_id());
    });
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

void ProjectInteractor::on_selected_bed_instances_changed(
    Domain::SelectionId project_id,
    const Scene::BedSelection& selection
)
{
    const Domain::BedRef last_selected_bed{selection.last_selected_bed()};
    const Domain::SelectionId container_id{last_selected_bed.config_container_id};

    if (container_id != m_selection.config_container_id)
        do_select_config_container(container_id);
}

void ProjectInteractor::on_slicing_input_changed(const Domain::BedRef& bed_instance)
{
    auto& project = selected_project();
    const Domain::BedInstance* instance{
        project.find_bed_instance_by_id(bed_instance.instance_id)
    };
    ASSERT(instance);
    const auto* config_container{
        project.find_config_container(bed_instance.config_container_id)
    };
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

    invoke_listeners<ISelectedProjectChangedListener>([project_id](auto* l) {
        l->on_selected_project_changed(project_id);
    });
}

void ProjectInteractor::do_select_config_container(Domain::SelectionId container_id)
{
    m_selection.config_container_id = container_id;
    Domain::SelectionId project_id  = m_selection.project_id;
    invoke_listeners<ISelectedConfigContainerChangedListener>([container_id, project_id](auto* l) {
        l->on_selected_config_container_changed(project_id, container_id);
    });
}

Domain::SelectionId ProjectInteractor::add_project(Domain::Project&& p)
{
    auto& projects                 = m_workbench.projects();
    Domain::SelectionId project_id = m_workbench.next_project_id();
    projects.emplace(project_id, std::move(p));
    invoke_listeners<IProjectsChangedListener>([project_id](auto* l) {
        l->on_project_added(project_id);
    });
    // select project
    do_select_project(project_id);
    return project_id;
}

void ProjectInteractor::remove_project(Domain::SelectionId project_id)
{
    auto& projects = m_workbench.projects();
    auto it        = projects.find(project_id);

    ASSERT(it != projects.end());

    invoke_listeners<IProjectsChangedListener>([project_id](auto* l) {
        l->on_project_will_be_removed(project_id);
    });

    it = projects.erase(it);

    invoke_listeners<IProjectsChangedListener>([project_id](auto* l) {
        l->on_project_removed(project_id);
    });

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
        PrintHost::get_export_format_from_extension(
            boost::filesystem::path(filename).extension().string()
        )
    };
    m_print_host_interactor.upload_gcode(std::move(config), std::move(data));
}

void ProjectInteractor::do_upload_connect(const Domain::SlicingId id, const std::string& connect_msg)
{
    const std::optional<FDMResultRef> fdm_result{m_fdm_result_cache.get_result(id)};
    if (!fdm_result)
        return;

    PrintHost::PrintHostConfig config{
        Domain::PrintHostType::PrusaConnect,
        Network::ServiceConfig::instance().connect_url()
    };
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
        PrintHost::get_export_format_from_extension(
            boost::filesystem::path(filename).extension().string()
        )
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

} // namespace Slic3r::Biz
