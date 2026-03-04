#include "Slic3r/Biz/ProjectInteractor.hpp"

#include <Slic3r/Assert.hpp>
#include <Slic3r/Domain/Workbench.hpp>
#include <Slic3r/Domain/Project.hpp>
#include <Slic3r/Domain/Bed.hpp>
#include "Slic3r/Domain/Model.hpp"

#include "Slic3r/Biz/ISelectedProjectChangedListener.hpp"
#include "Slic3r/Biz/IProjectsChangedListener.hpp"
#include "Slic3r/Biz/IMessageDialogProvider.hpp"
#include "Slic3r/Biz/UserAccount/ConnectUtils.hpp"
#include "Slic3r/Biz/Platform/JobManager/JobManager.hpp"
#include "Slic3r/Biz/FileLoadingLogic.hpp"
#include "Slic3r/Biz/Scene/BedFactory.hpp"
#include "Slic3r/Biz/Algorithms/Point.hpp"

#include "Slic3r/Directories.hpp"

#include <Slic3r/Biz/I18N/I18N.hpp> // translations
#include <boost/filesystem/path.hpp>

namespace Slic3r::Biz {
Domain::SelectionId ProjectInteractor::Selection::config_container_id() const
{
    if (project_id == Domain::INVALID_ID)
        return Domain::INVALID_ID;
    auto it = project_config_container.find(project_id);
    if (it == project_config_container.end())
        return Domain::INVALID_ID;
    return it->second;
}

void ProjectInteractor::Selection::set_config_container_id(Domain::SelectionId container_id)
{
    project_config_container[project_id] = container_id;
}

const Domain::Project& ProjectInteractor::selected_project() const
{
    ASSERT(m_selection.project_id != Domain::INVALID_ID);
    return m_workbench.project(m_selection.project_id);
}

Domain::Project& ProjectInteractor::selected_project()
{
    ASSERT(m_selection.project_id != Domain::INVALID_ID);
    return m_workbench.project(m_selection.project_id);
}

bool ProjectInteractor::project_exists(size_t project_id) const {
    for (const auto& [id,_] : m_workbench.projects()) {
        if (id == project_id)
            return true;
    }
    return false;
}

const Domain::Project& ProjectInteractor::project(size_t project_id) const {
    ASSERT(project_exists(project_id));
    return m_workbench.project(project_id);
}

Domain::Project& ProjectInteractor::project(size_t project_id)
{
    ASSERT(project_exists(project_id));
    return m_workbench.project(project_id);
}

void ProjectInteractor::initialize_bed(Domain::SelectionId project_id, Domain::SelectionId config_container_id,
    Domain::BedContainer& bed_container)
{
    Domain::Project& project{ m_workbench.project(project_id) };
    Domain::ConfigContainer* config_container = project.find_config_container(config_container_id);
    DEBUG_ASSERT(config_container != nullptr);
    Domain::Bed& bed{ 
        Scene::get_or_create_bed(
            bed_container, *config_container, resources_dir(), project_id, config_container_id,
            [this](Domain::SelectionId project_id, Domain::SelectionId config_container_id) {
                return m_preset_interactor.system_preset_bed_shape(project_id, config_container_id);
            }
        )
    };
    config_container->set_bed(bed);
    m_scene_interactor.add_bed_instance(config_container_id);
}

Domain::SelectionId ProjectInteractor::new_project()
{
    return new_project_with_modification([](auto& _){});
}

Domain::SelectionId ProjectInteractor::new_project_with_modification(
    const std::function<void(Domain::Project&)>& modifier
)
{
    Domain::Project project;

    project.config_containers().emplace_back(std::make_unique<Domain::ConfigContainer>());
    auto& config_container = *project.config_containers().front();
    Domain::SelectionId project_id;
    {
        InvokeLaterBag bag;
        project_id = add_project(std::move(project), bag);
        m_preset_interactor.initialize_config_container_with_default(config_container);

        Domain::Project& added_project{m_workbench.project(project_id)};
        initialize_bed(project_id, config_container.id().id, added_project.bed_container());

        modifier(added_project);
        m_scene_interactor.prepare_added_project(project_id);
    }
    invoke_listeners<IProjectsChangedListener>([project_id](auto* l) {
        l->on_project_loaded(project_id);
    });

    return project_id;
}

void ProjectInteractor::load_project(const boost::filesystem::path& file_path)
{
    auto report_error{
        [this](const std::string& description)
        {
            SPDLOG_ERROR(description);
            invoke_listeners<IProjectsChangedListener>([&description](IProjectsChangedListener* l)
            {
                l->on_project_load_failed(description);
            });
        }
    };

    auto on_result{
        [this, file_path, &report_error](Domain::Project&& project)
        {
            if (project.config_containers().empty() && project.model().objects.empty())
                return;

            Domain::SelectionId project_id;
            {
                InvokeLaterBag bag;
                auto original_project_id = m_selection.project_id;
                project_id = add_project(std::move(project), bag);
                Domain::Project& added_project{m_workbench.project(project_id)};

                for (auto& config_container : added_project.config_containers()) {
                    auto result = m_preset_interactor.load_selected_preset_from_3mf(
                        project_id,
                        config_container->mutable_selected_preset()
                    );
                    if (!result.has_value()) {
                        // clean up project state
                        select_project(original_project_id);
                        remove_project(project_id);

                        // invoke error listener
                        report_error(result.error());

                        // and quit
                        return;
                    }
                }

                if (added_project.config_containers().empty()) {
                    added_project.config_containers().emplace_back(std::make_unique<Domain::ConfigContainer>());
                    auto cc = added_project.config_containers().back().get();
                    m_preset_interactor.initialize_config_container_with_default(*cc);
                    initialize_bed(project_id, cc->id().id, added_project.bed_container());
                }
                // Ensure bed selection is valid for the config container.
                const Domain::ConfigContainer& config_container{
                    *added_project.config_containers().front()
                };
                m_scene_interactor.bed_selection().select_one(
                    {config_container.id().id, config_container.bed_instances().front()->id().id}
                );

                do_select_config_container(added_project.config_containers().front()->id().id);

                m_scene_interactor.prepare_added_project(project_id);

                set_project_dir(project_id, file_path);
            }

            invoke_listeners<IProjectsChangedListener>([project_id](auto* l) {
                l->on_project_loaded(project_id);
            });
        }
    };

    auto on_error{[&](std::exception_ptr eptr)
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
        report_error(description);
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

namespace {
std::string get_file_name(const std::string& file_path)
{
    size_t pos_last_delimiter = file_path.find_last_of("/\\");
    size_t pos_point = file_path.find_last_of('.');
    size_t offset = pos_last_delimiter + 1;
    size_t count = pos_point - pos_last_delimiter - 1;
    return file_path.substr(offset, count);
}
using SvgFile = Domain::EmbossShape::SvgFile;
using SvgFiles = std::vector<SvgFile*>;
std::string create_unique_3mf_filepath(const std::string& file, const SvgFiles svgs)
{
    // const std::string MODEL_FOLDER = "3D/"; // copy from file 3mf.cpp
    std::string path_in_3mf = "3D/" + file + ".svg";
    size_t suffix_number = 0;
    bool is_unique = false;
    do {
        is_unique = true;
        path_in_3mf = "3D/" + file + ((suffix_number++) ? ("_" + std::to_string(suffix_number)) : "") + ".svg";
        for (SvgFile* svgfile : svgs) {
            if (svgfile->path_in_3mf.empty())
                continue;
            if (svgfile->path_in_3mf.compare(path_in_3mf) == 0) {
                is_unique = false;
                break;
            }
        }
    } while (!is_unique);
    return path_in_3mf;
}

bool set_by_local_path(SvgFile& svg, const SvgFiles& svgs)
{
    // Try to find already used svg file
    for (SvgFile* svg_ : svgs) {
        if (svg_->path_in_3mf.empty())
            continue;
        if (svg.path.compare(svg_->path) == 0) {
            svg.path_in_3mf = svg_->path_in_3mf;
            return true;
        }
    }
    return false;
}

/**
    @brief Function to secure private data before store to 3mf
    @param model    - Data(also private) to clean before publishing
    @param messager - Ability to polite ask users
**/
void publish(Domain::Model& model, IMessageDialogProvider* messager) {

    // SVG file publishing
    bool exist_new_svg = false;
    SvgFiles svgs;
    for (Domain::ModelObject* mo : model.objects) {
        for (Domain::ModelVolume* mv : mo->volumes) {
            if (!mv->emboss_shape.has_value())
                continue; // do not contain emboss shape
            if (!mv->emboss_shape->svg_file.has_value())
                continue; // do not contain svg file
            SvgFile* svg = &(*mv->emboss_shape->svg_file);
            if (svg->path_in_3mf.empty())
                exist_new_svg = true;
            svgs.push_back(svg);
        }
    }

    if (exist_new_svg && messager != nullptr) {
        std::string message_title = Biz::_u8L("SVG file obfuscate");
        std::string message_text = Biz::_u8L("Are you sure you want to store original SVGs with their local paths into the 3MF file ? \n"
            "If you hit 'NO', all SVGs in the project will not be editable any more.");
        IMessageDialogProvider::YesNoCallback callback = [&model](bool answer) {
            if (answer == false) {
                for (Domain::ModelObject* mo : model.objects) {
                    for (Domain::ModelVolume* mv : mo->volumes) {
                        if (mv->emboss_shape.has_value() &&
                            mv->emboss_shape->svg_file.has_value() &&
                            mv->emboss_shape->svg_file->path_in_3mf.empty()) {
                            mv->emboss_shape.reset(); // became regular volume
                        }
                    }
                }
            }
        };
        messager->show_yesno_dialog(message_title, message_text, callback);
    }

    for (SvgFile* svgfile : svgs) {
        if (!svgfile->path_in_3mf.empty())
            continue; // already suggested path (previous save)
        // create unique name for svgs, when local path differ
        std::string filename = "unknown";
        if (!svgfile->path.empty()) {
            if (set_by_local_path(*svgfile, svgs))
                continue;
            // check whether original filename is already in:
            filename = get_file_name(svgfile->path);
        }
        svgfile->path_in_3mf = create_unique_3mf_filepath(filename, svgs);
    }
}
}

void ProjectInteractor::save_project(const boost::filesystem::path& file_path, const Store3mfParam& params)
{
    auto& selected_project = this->selected_project();
    selected_project.increment_version();
    selected_project.set_file_name(file_path.stem().string());
    publish(selected_project.model(), m_dialog_provider);
    store_3mf(file_path.string(), selected_project, params);

    selected_project.directory_storage().set_project_dir(file_path);

    invoke_listeners<IProjectsChangedListener>(
        [this](auto* l) { l->on_project_changed(selected_project_id()); }
    );
}

void ProjectInteractor::select_project(Domain::SelectionId project_id)
{
    if (project_id != m_selection.project_id) {
        {
            InvokeLaterBag bag;
            do_select_project(project_id, bag);
        }

        if (m_selection.config_container_id() == Domain::INVALID_ID) {
            const auto& projects         = m_workbench.projects();
            const auto& config_container = projects.at(project_id).config_containers().front();
            const Domain::SelectionId first_container_id = config_container->id().id;
            do_select_config_container(first_container_id);
        } else {
            // TODO: remove this once the right side panel elements are correctly storing selected
            // config container per project
            auto container_id = m_selection.config_container_id();
            invoke_listeners<ISelectedConfigContainerChangedListener>(
                [container_id, project_id](auto* l)
                { l->on_selected_config_container_changed(project_id, container_id); }
            );
        }
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

    if (container_id != m_selection.config_container_id())
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

void ProjectInteractor::on_colors_changed(
    Domain::SelectionId config_container_id,
    const std::vector<Domain::ColorRGB>& /*colors*/
)
{
    auto& project = selected_project();
    const auto* config_container = project.find_config_container(config_container_id);
    if (!config_container)
        return;

    const auto& selected_preset = config_container->selected_preset();
    for (const auto& bed_instance : config_container->bed_instances()) {
        m_slicing_interactor.update_process(
            project.model(),
            project.metadata(),
            selected_preset.metadata(),
            config_container->print_config(),
            *bed_instance
        );
    }
}

void ProjectInteractor::do_select_project(Domain::SelectionId project_id, InvokeLaterBag& bag)
{
    m_selection.project_id = project_id;

    invoke_listeners<ISelectedProjectChangedListener>(
        [project_id](auto* l) { l->on_selected_project_changed(project_id); }
    );

    bag.add(
        [this, project_id]
        {
            invoke_listeners<ISelectedProjectChangedListener>(
                [project_id](auto* l) { l->on_selected_project_changed_final(project_id); }
            );
        }
    );
}

void ProjectInteractor::do_select_config_container(Domain::SelectionId container_id)
{
    m_selection.set_config_container_id(container_id);
    Domain::SelectionId project_id  = m_selection.project_id;
    invoke_listeners<ISelectedConfigContainerChangedListener>(
        [container_id, project_id](auto* l)
        { l->on_selected_config_container_changed(project_id, container_id); }
    );
}

Domain::SelectionId ProjectInteractor::add_project(Domain::Project&& p)
{
    InvokeLaterBag bag;
    return add_project(std::move(p), bag);
}

Domain::SelectionId ProjectInteractor::add_project(Domain::Project&& p, InvokeLaterBag& bag)
{
    auto& projects                 = m_workbench.projects();
    Domain::SelectionId project_id = m_workbench.next_project_id();
    projects.emplace(project_id, std::move(p));
    invoke_listeners<IProjectsChangedListener>(
        [project_id](auto* l) { l->on_project_added_uninitialized(project_id); }
    );
    // select project
    do_select_project(project_id, bag);
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

void ProjectInteractor::do_result_export_inner(const Domain::SlicingId id, PhysicalPrinter::PhysicalPrinterConfig&& print_host_config, PrintHost::PrintHostJobData&& job_data)
{
    // Find confing container with matching bed instance id.
    const Domain::ConfigContainer* config_container = nullptr;
    for (const auto& cc : m_workbench.project(id.project_id).config_containers()) {
        for (const auto& bi : cc->bed_instances()) {
            if (bi->id() == id.bed_instance_id) {
                config_container = cc.get();
                break;
            }
        }
        if (config_container != nullptr) {
            break;
        }
    }
    ASSERT(config_container);
    Domain::PrinterTechnology tech = config_container->selected_preset().hw_config.technology;
    if (tech == Domain::PrinterTechnology::FFF) {
        const std::optional<FDMResultRef> fdm_result{m_fdm_result_cache.get_result(id)};
        ASSERT(fdm_result);
        job_data.data_ptr = fdm_result.value().get().const_gcode();
        m_result_export_interactor.perform(std::move(print_host_config), std::move(job_data));
    } else if (tech == Domain::PrinterTechnology::SLA) {

        const std::optional<SLAResultRef> sla_result{m_sla_result_cache.get_result(id)};
        ASSERT(sla_result);
        job_data.data_ptr = sla_result.value().get().export_data;
        m_result_export_interactor.perform(std::move(print_host_config), std::move(job_data));

    } else {
        ASSERT(false);
    }
}

void ProjectInteractor::do_result_export(const Domain::SlicingId id, const boost::filesystem::path& dest_path)
{
    set_output_dir(id.project_id, dest_path);
    PhysicalPrinter::PhysicalPrinterConfig config;
    config.payload = PhysicalPrinter::FileSystemExport{};
    PrintHost::PrintHostJobData data{
        std::monostate{},
        dest_path,
        PrintHost::get_export_format_from_extension(dest_path.extension().string())
    };
    do_result_export_inner(id, std::move(config), std::move(data));
}

void ProjectInteractor::do_result_upload(const Domain::SlicingId id, const std::string& filename)
{
    PhysicalPrinter::PhysicalPrinterConfig config {m_physical_printer_interactor.selected_physical_printer_data()};
    boost::filesystem::path dest_path(filename);
    PrintHost::PrintHostJobData data{
        std::monostate{},
        dest_path,
        PrintHost::get_export_format_from_extension(dest_path.extension().string())
    };
    do_result_export_inner(id, std::move(config), std::move(data));
}

void ProjectInteractor::do_result_upload_connect(const Domain::SlicingId id, const std::string& connect_msg)
{
    PhysicalPrinter::PhysicalPrinterConfig config;
    config.host = Network::ServiceConfig::instance().connect_url();
    PhysicalPrinter::ConnectUpload auth;
    auth.access_token = m_user_account_interactor.access_token();
    config.payload = std::move(auth);

    std::string filename;
    std::string body_json;
    if (!UserAccount::ConnectUtils::config_from_json(connect_msg, config, filename, body_json)) {
        SPDLOG_ERROR("Upload to Connect has failed - failed to read Connect message.");
        return;
    }
    PrintHost::PrintHostJobData data{
        std::monostate{},
        filename,
        PrintHost::get_export_format_from_extension(boost::filesystem::path(filename).extension().string())
    };
    data.request_body_json = std::move(body_json);
    do_result_export_inner(id, std::move(config), std::move(data));
}

void ProjectInteractor::on_model_downloaded(const std::vector<boost::filesystem::path>& paths, bool in_new_project)
{
    ASSERT(!paths.empty());
    boost::filesystem::path file_path = paths.front();
    std::string ext_str = file_path.extension().string();
    // file path could have locale dependent characters, do not use tolower
    bool load_as_single_project = paths.size() ==1 && (ext_str == ".3mf" || ext_str == ".3MF");
    if (in_new_project && load_as_single_project) {
        load_project(paths.front());
        return;
    }

    if (in_new_project) {
        new_project();
    }
    load_models_to_project(paths);
}

std::string ProjectInteractor::get_project_name(Domain::SelectionId project_id) const
{
    auto it = m_workbench.projects().find(project_id);
    ASSERT(it != m_workbench.projects().end());

    return it->second.file_name();
}

boost::filesystem::path ProjectInteractor::project_dir(Domain::SelectionId project_id, const std::string& app_config_val) const
{
    auto it = m_workbench.projects().find(project_id);
    ASSERT(it != m_workbench.projects().end());

    return it->second.directory_storage().project_dir(app_config_val);
}

void ProjectInteractor::set_project_dir(Domain::SelectionId project_id, const boost::filesystem::path& path)
{
    auto it = m_workbench.projects().find(project_id);
    ASSERT(it != m_workbench.projects().end());

    it->second.directory_storage().set_project_dir(path);
}

boost::filesystem::path ProjectInteractor::output_dir(Domain::SelectionId project_id, bool only_removable, const std::string& app_config_val) const
{
    auto it = m_workbench.projects().find(project_id);
    ASSERT(it != m_workbench.projects().end());

    if (only_removable) {
        return m_removable_drive_service.get_path_on_removable_drive(it->second.directory_storage().output_dir(app_config_val));
    }
    return it->second.directory_storage().output_dir(app_config_val);
}

void ProjectInteractor::set_output_dir(Domain::SelectionId project_id, const boost::filesystem::path& path)
{
    auto it = m_workbench.projects().find(project_id);
    ASSERT(it != m_workbench.projects().end());

    it->second.directory_storage().set_output_dir(path);
}

void ProjectInteractor::load_models_to_project(std::vector<boost::filesystem::path> paths)
{
    const auto& proj            = m_workbench.project(selected_project_id());
    Domain::BedRef selected_bed = scene_interactor().bed_selection().last_selected_bed();
    const Domain::ConfigContainer* cc =
        proj.find_config_container(selected_bed.config_container_id);
    const Domain::BedInstance& inst = cc->find_bed_instance(selected_bed.instance_id);
    int slot_count             = cc->selected_preset().hw_config.material_slot_count();
    FileLoadingLogic::import_files_and_add_to_scene(
        paths,
        slot_count,
        scene_interactor(),
        cc->bed().center() + Biz::Algorithms::Point::to_2d(inst.transformation.get_offset()),
        m_dialog_provider
    );

    set_project_dir(selected_project_id(), paths.front());
}

Domain::SelectionId ProjectInteractor::add_config_container()
{
    auto& p = m_workbench.project(m_selection.project_id);
    p.config_containers().emplace_back(std::make_unique<Domain::ConfigContainer>());
    auto& cc = *p.config_containers().back();

    m_preset_interactor.initialize_config_container_with_selected(cc);
    auto id = cc.id().id;
    initialize_bed(m_selection.project_id, id, p.bed_container());

    select_config_container(id);
    return id;
}

Domain::SelectionId ProjectInteractor::duplicate_config_container(Domain::SelectionId config_container_id)
{
    select_config_container(config_container_id); 
    return add_config_container();
}

void ProjectInteractor::remove_config_container(Domain::SelectionId config_container_id)
{
    auto& project = m_workbench.project(m_selection.project_id);
    auto& ccs = project.config_containers();

    ASSERT(ccs.size() > 1);

    auto it = std::find_if(ccs.begin(), ccs.end(), [config_container_id](const auto& cc_ptr) { return cc_ptr->id().id == config_container_id; });
    ASSERT(it != ccs.end());

    // Remove all beds from container before its will be erased
    Domain::ConfigContainer* cc_ptr = it->get();
    while (!cc_ptr->bed_instances().empty()) {
        const size_t bed_id = cc_ptr->bed_instances().back().get()->id().id;
        scene_interactor().remove_bed_instance(
            {.config_container_id = config_container_id, .instance_id = bed_id},
            true
        );
    }

    it = ccs.erase(it);
    if (it == ccs.end())
        it = ccs.begin();
    select_config_container((*it)->id().id);

    scene_interactor().on_removed_config_container(project);
}

void ProjectInteractor::select_config_container(Domain::SelectionId container_id)
{
    if (container_id != m_selection.config_container_id())
        do_select_config_container(container_id);
}

} // namespace Slic3r::Biz
