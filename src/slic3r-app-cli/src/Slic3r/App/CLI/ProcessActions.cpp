#include "Slic3r/App/CLI/ProcessActions.hpp"

#include "Slic3r/App/CLI/LoadPrintData.hpp"
#include "Slic3r/App/CLI/ProcessTransform.hpp"
#include "Slic3r/App/CLI/ProfilesSharingUtils.hpp"
#include "Slic3r/App/Init.hpp"
#include "Slic3r/App/Platform/StdMainThreadDispatcher.hpp"
#include "Slic3r/App/PresetUpdaterCLI.hpp"
#include "Slic3r/Biz/Algorithms/Model.hpp"
#include "Slic3r/Biz/Config/ConfigSerialize.hpp"
#include "Slic3r/Biz/Format/3mf.hpp"
#include "Slic3r/Biz/Format/STL.hpp"
#include "Slic3r/Biz/Platform/JobManager/JobManager.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/SecretStoreDummy.hpp"
#include "Slic3r/Biz/Slicing/SlicingInteractor.hpp"
#include "Slic3r/Biz/Utils/CopyFile.hpp"
#include "Slic3r/Biz/ResultExport/ExportNameParser.hpp"
#include "Slic3r/Directories.hpp"
#include "Slic3r/Domain/FullConfigFDM.hpp"
#include "Slic3r/Domain/FullConfigSLA.hpp"
#include "Slic3r/Domain/Image.hpp"
#include "Slic3r/Domain/Model.hpp"
#include "Slic3r/Domain/PixelFormat.hpp"
#include "Slic3r/Domain/Project.hpp"

#include <string>
#include <cstring>
#include <iostream>
#include <sstream>
#include <set>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/nowide/iostream.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/nowide/filesystem.hpp>

#include <nlohmann/json.hpp>

#include <spdlog/spdlog.h>

#include <arrange-wrapper/ModelArrange.hpp>

#include "Slic3r/Biz/Format/OBJ.hpp"
#include "libslic3r/IThumbnailImageGenerator.hpp"
#include "libslic3r/MultipleBeds.hpp"

namespace fs = boost::filesystem;

using namespace Slic3r;
using namespace Slic3r::Biz;

using Slic3r::Domain::Model;
using Slic3r::Domain::Project;
using Slic3r::Domain::Vec2crd;
using Slic3r::Domain::Vec2d;
using Slic3r::Domain::Vec2ds;

namespace Slic3r::App::CLI {




class CLIThumbnailImageGenerator : public Slicing::IThumbnailImageGenerator
{
public:
    CLIThumbnailImageGenerator() = default;
    explicit CLIThumbnailImageGenerator(const std::vector<std::string>& input_files)
    {
        if (input_files.size() == 1 && boost::iends_with(input_files[0], ".3mf")) {
            m_filename = input_files[0];
        }
    }

    std::future<Slicing::ThumbnailImageResults> enqueue_thumbnail_requests(
        const Slicing::ThumbnailImageRequests& requests
    ) override
    {
        std::promise<Slicing::ThumbnailImageResults> promise;
        std::future<Slicing::ThumbnailImageResults> result{promise.get_future()};
        if (m_filename.empty()) {
            promise.set_value(Slicing::ThumbnailImageResults{});
            return result;
        }

        // Create a list of all sizes that we need to generate.
        std::vector<Domain::Size> sizes;
        for (const auto& request : requests) {
            for (const auto& size : request.params.sizes) {
                sizes.emplace_back(size);
            }
        }

        // Now actually generate the thumbnails:
        std::vector<Domain::Image> source_images = get_thumbnail_images_from_3mf(m_filename, sizes);
        
        if (source_images.empty() || source_images.size() != sizes.size()
         || std::any_of(source_images.begin(), source_images.end(),
             [](const Domain::Image& img) { return img.width() == 0 || img.height() == 0; })
            ) {
            promise.set_value(Slicing::ThumbnailImageResults{});
            return result;
        }

        Slicing::ThumbnailImageResults results;
        size_t j=0;
        for (const auto& request : requests) {
            Slicing::ThumbnailImageResult thumbnail_result;
            thumbnail_result.type = request.type;
            thumbnail_result.project_id = request.params.project_id;
            thumbnail_result.bed_instance_id = request.params.bed_instance_id;

            for (size_t i = 0; i < request.params.sizes.size(); ++i) {
                thumbnail_result.images.push_back(source_images[j++]);
                ASSERT(thumbnail_result.images.back().width() == request.params.sizes[i].width);
                ASSERT(thumbnail_result.images.back().height() == request.params.sizes[i].height);
            }
            results.push_back(std::move(thumbnail_result));
        }
        ASSERT(j == source_images.size());
        promise.set_value(std::move(results));
        return result;
    }

    void handle_enqueued_requests() override {}
private:
    std::string m_filename;
};

struct SlicingStatusChangeListener : Slicing::IStatusListener
{
    std::promise<std::pair<Domain::SlicingId, Slicing::StatusUpdate>> promise_slicing_finished;

    void on_status_changed(
        const Slicing::StatusUpdate status_update,
        const Domain::SlicingId slicing_id
    ) override
    {
        if ((status_update.code.has_value()
             && status_update.code == Biz::Slicing::StatusCode::Finished)
            || !status_update.errors_to_append.empty())
        {
            promise_slicing_finished.set_value(std::make_pair(slicing_id, status_update));
        }
    }
};

struct ExportFinishedJobManagerStatusListener : public Biz::Platform::JobManager::IJobManagerStatusChangedListener
{
    std::promise<std::optional<std::string>> promise_export_error;

    void on_job_manager_status_changed(
        const Slic3r::Biz::Platform::JobManager::JobManagerStatus& job_manager_status
    ) override
    {
        for (const auto& [job_name, progress] : job_manager_status) {
            if (!job_name.starts_with("printhost")) {
                continue;
            }

            std::string payload_message;
            if (const auto* payload =
                    std::any_cast<Biz::PrintHost::PrintHostJobProgressPayload>(&progress.progress_detail.payload))
            {
                payload_message = payload->message;
            }

            if (progress.status == Slic3r::Domain::JobStatus::Failed)
            {
                promise_export_error.set_value(payload_message);
            } else if (progress.status == Slic3r::Domain::JobStatus::Finished)
            {
                promise_export_error.set_value(std::nullopt);
            }
        }
    }
};

static std::string output_filename_and_path(
    const Project& project,
    const Domain::ConfigPack& config_pack,
    const std::optional<std::string>& output_path
)
{
    const boost::filesystem::path model_proposed_filename_and_path{
        Algorithms::Model::propose_export_file_name_and_path(project.model())
    };

    const std::string extension = [&config_pack]() -> std::string
    {
        if (std::holds_alternative<Domain::ConfigPackSLA>(config_pack)) {
            return ".sl1";
        } else {
            const Domain::ConfigPackFDM& config_pack_fdm =
                std::get<Domain::ConfigPackFDM>(config_pack);
            if (config_pack_fdm.printer.items.opt("binary_gcode").get<bool>()) {
                return ".bgcode";
            } else {
                return ".gcode";
            }
        }
    }();

    const boost::filesystem::path output_dir =
        [&output_path, &model_proposed_filename_and_path]() -> boost::filesystem::path
    {
        if (output_path.has_value()) {
            const boost::filesystem::path path(output_path.value());
            if (boost::filesystem::is_directory(path)) {
                return path;
            } else if (!path.parent_path().empty()) {
                return path.parent_path();
            }
        }

        if (!model_proposed_filename_and_path.parent_path().empty()) {
            return model_proposed_filename_and_path.parent_path();
        }

        return fs::current_path();
    }();

    const std::string filename =
        [&project, &output_path, &model_proposed_filename_and_path]() -> std::string
    {
        if (output_path.has_value()) {
            const boost::filesystem::path path(output_path.value());
            if (!boost::filesystem::is_directory(path) && !path.filename().empty()) {
                return path.filename().string();
            }
        }

        if (!project.file_name().empty()) {
            return project.file_name();
        }

        return model_proposed_filename_and_path.filename().string();
    }();

    return (output_dir / filename).replace_extension(extension).string();
}

std::optional<std::string> slice_single_model_project(
    Domain::Project&& project_to_slice,
    ProjectInteractor& project_interactor,
    const Domain::ConfigPack& config_pack,
    const std::optional<std::string>& output_path
)
{
    Domain::Model& model = project_to_slice.model();
    // Remove all projects before slicing another one.
    const Domain::Workbench& workbench = project_interactor.workbench();
    for (const Domain::SelectionId selection_id : workbench.projects() | std::views::keys) {
        project_interactor.remove_project(selection_id);
    }

    Biz::Platform::IMainThreadDispatcher& dispatcher =
        Biz::Platform::PlatformServices::instance().main_thread_dispatcher();

    project_interactor.new_project_with_modification(
        [&](Project& project)
        {
            project.model() = std::move(model);
            project.set_file_name(project_to_slice.file_name());

            // Apply the provided config_pack.
            Domain::ConfigContainer& config_container = *project.config_containers().front();
            Domain::Preset::SelectedPresetMetadata metadata =
                config_container.selected_preset().metadata();
            if (std::holds_alternative<Domain::ConfigPackSLA>(config_pack)) {
                metadata.hw_config.technology = Domain::PrinterTechnology::SLA;
            }

            config_container.mutable_selected_preset() =
                Domain::Preset::SelectedPreset::make(metadata, config_pack);
        }
    );
    const Project& project = project_interactor.selected_project();

    Slicing::SlicingInteractor& slicing_interactor = project_interactor.slicing_interactor();
    SlicingStatusChangeListener slicing_status_change_listener;
    slicing_interactor.add_listener<Biz::Slicing::IStatusListener>(&slicing_status_change_listener);

    const Domain::SlicingId slicing_id = project_interactor.selected_bed_slicing_id();
    slicing_interactor.slice_bed(slicing_id);

    const Slicing::StatusUpdate& slicing_status_update = [&slicing_status_change_listener,
                                             &dispatcher]() -> Slicing::StatusUpdate
    {
        std::future<std::pair<Domain::SlicingId, Slicing::StatusUpdate>> future_slicing_status_update =
            slicing_status_change_listener.promise_slicing_finished.get_future();
        while (future_slicing_status_update.wait_for(std::chrono::milliseconds(1))
               != std::future_status::ready)
        {
            dispatcher.dispatch_enqueued();
        }

        return future_slicing_status_update.get().second;
    }();

    std::string dest_path = output_filename_and_path(project, config_pack, output_path);

    if (slicing_status_update.code == Biz::Slicing::StatusCode::Finished) {
        Biz::Platform::PlatformServices& platform_services =
            Biz::Platform::PlatformServices::instance();
        platform_services.set_job_manager(
            std::make_unique<Biz::Platform::JobManager::JobManager>(dispatcher)
        );
        ExportFinishedJobManagerStatusListener export_finished_listener;
        platform_services.job_manager().add_listener<Biz::Platform::JobManager::IJobManagerStatusChangedListener>(&export_finished_listener);

        Biz::ExportNameParser::ExportNameData name_data;
        try {
           name_data = Biz::ExportNameParser::parse_export_name(project_interactor);
        } catch (const Slic3r::PlaceholderParserError& e) {
            SPDLOG_ERROR("Failed to parse output filename: {}", e.what());
        }

        boost::filesystem::path export_path;
        if (name_data.filename.empty()) {
            export_path = boost::filesystem::path(dest_path);
        } else {
            export_path = boost::filesystem::path(dest_path).parent_path() / name_data.filename;
            // Store back to dest_path, it is used later.
            dest_path = export_path.string();
        }

        project_interactor.do_result_export(slicing_id, export_path);

        std::optional<std::string> export_error = [&export_finished_listener, &dispatcher]()
        {
            std::future<std::optional<std::string>> future_slicing_status =
                export_finished_listener.promise_export_error.get_future();
            while (future_slicing_status.wait_for(std::chrono::milliseconds(1))
                   != std::future_status::ready)
            {
                dispatcher.dispatch_enqueued();
            }

            return future_slicing_status.get();
        }();

        if (export_error.has_value()) {
            return export_error.value();
        }
    } else if (slicing_status_update.code == Biz::Slicing::StatusCode::Empty) {
        return "Nothing to print for "
            + dest_path
            + " . Either the print is empty or no object is fully inside the print volume.";
    } else {
        std::ostringstream oss;
        oss << slicing_status_update;
        return oss.str();
    }

    const std::string dest_path_final = [&output_path, &dest_path]() -> std::string
    {
        if (output_path.has_value()) {
            const boost::filesystem::path path(output_path.value());
            if (!boost::filesystem::is_directory(path)) {
                return path.string();
            }
        }

        return dest_path;
    }();

    if (dest_path != dest_path_final) {
        if (Utils::rename_file(dest_path, dest_path_final)) {
            return "Renaming file " + dest_path + " to " + dest_path_final + " failed";
        }

        boost::nowide::cout << "Slicing result exported to " << dest_path_final << std::endl;
    } else {
        boost::nowide::cout << "Slicing result exported to " << dest_path << std::endl;
    }

    return std::nullopt;
}

static bool has_profile_sharing_action(const InitParams& init_params)
{
    return init_params.action.query_printer_models
        || init_params.action.query_print_tool_filament_profiles;
}

bool has_full_config_from_profiles(const InitParams& init_params)
{
    const InputParams& input = init_params.input;

    return !has_profile_sharing_action(init_params)
        && ((input.print_profile_preset.has_value() && !input.print_profile_preset->empty())
            || !input.material_profile_presets.empty()
            || !input.tool_profile_presets.empty()
            || (input.printer_profile_preset.has_value() && !input.print_profile_preset->empty()));
}

bool process_profiles_sharing(const InitParams& init_params)
{
    if (!has_profile_sharing_action(init_params)) {
        return false;
    }

    std::string ret;
    if (init_params.action.query_printer_models) {
        ret = get_json_printer_models(get_printer_technology(init_params));
    } else if (init_params.action.query_print_tool_filament_profiles) {
        if (init_params.input.printer_profile_preset.has_value()) {
            const std::string& printer_profile = init_params.input.printer_profile_preset.value();
            ret = get_json_print_tool_filament_profiles(printer_profile);
            if (ret.empty()) {
                boost::nowide::cerr
                    << "query-print-tool-filament-profiles error: Printer profile '"
                    << printer_profile
                    << "' wasn't found among installed printers."
                    << std::endl
                    << "Or the request can be wrong."
                    << std::endl;
                return true;
            }
        } else {
            boost::nowide::cerr
                << "query-print-tool-filament-profiles error: This action requires set 'printer-profile' option"
                << std::endl;
            return true;
        }
    }

    if (ret.empty()) {
        boost::nowide::cerr << "Wrong request" << std::endl;
        return true;
    }

    // use --output when available
    if (init_params.misc.output.has_value()) {
        const std::string& cmdline_param = init_params.misc.output.value();
        // if we were supplied a directory, use it and append our automatically generated filename
        boost::filesystem::path cmdline_path(cmdline_param);
        boost::filesystem::path proposed_path =
            boost::filesystem::path(resources_dir()) / "out.json";
        if (boost::filesystem::is_directory(cmdline_path)) {
            proposed_path = (cmdline_path / proposed_path.filename());
        } else if (cmdline_path.extension().empty()) {
            proposed_path = cmdline_path.replace_extension("json");
        } else {
            proposed_path = cmdline_path;
        }

        const std::string file = proposed_path.string();
        boost::nowide::ofstream c;
        c.open(file, std::ios::out | std::ios::trunc);
        c << ret << std::endl;
        c.close();

        boost::nowide::cout << "Output for your request is written into " << file << std::endl;
    } else {
        printf("%s", ret.c_str());
    }

    return true;
}

namespace IO {
enum ExportFormat : int
{
    OBJ,
    STL,
    // SVG,
    TMF,
    Gcode
};
} // namespace IO

static std::string output_filepath(
    const Project& project,
    IO::ExportFormat format,
    const std::string& cmdline_param
)
{
    std::string ext;
    switch (format) {
    case IO::OBJ:
        ext = ".obj";
        break;
    case IO::STL:
        ext = ".stl";
        break;
    case IO::TMF:
        ext = ".3mf";
        break;
    default:
        assert(false);
        break;
    };

    boost::filesystem::path proposed_path = boost::filesystem::path(
        Algorithms::Model::propose_export_file_name_and_path(project.model(), ext)
    );

    if (!project.file_name().empty() && !proposed_path.parent_path().empty()) {
        proposed_path = proposed_path.parent_path() / (project.file_name() + ext);
    }

    // use --output when available
    if (!cmdline_param.empty()) {
        // if we were supplied a directory, use it and append our automatically generated filename
        boost::filesystem::path cmdline_path(cmdline_param);
        if (boost::filesystem::is_directory(cmdline_path)) {
            proposed_path = cmdline_path / proposed_path.filename();
        } else {
            proposed_path = cmdline_path;
        }
    }

    return proposed_path.string();
}

static bool export_projects(
    std::vector<Project>& projects,
    IO::ExportFormat format,
    const std::string& cmdline_param
)
{
    for (Domain::Project& project : projects) {
        const std::string path = output_filepath(project, format, cmdline_param);
        bool success           = false;
        switch (format) {
        case IO::OBJ: {
            success = Slic3r::Biz::store_obj(path.c_str(), &project.model());
            break;
        }
        case IO::STL: {
            success = store_stl(path, Algorithms::Model::flatten_to_mesh(project.model()), true);
            break;
        }
        case IO::TMF: {
            try {
                store_3mf(path, project, Store3mfParam{.fullpath_sources = false});

                success = true;
            } catch (boost::filesystem::filesystem_error&) {
                success = false;
            }

            break;
        }
        default: {
            assert(false);
            break;
        }
        }

        if (success) {
            std::cout << "File exported to " << path << std::endl;
        } else {
            SPDLOG_ERROR("File export to {} failed", path);
            return false;
        }
    }

    return true;
}

bool process_actions(
    const InitParams& init_params,
    const Domain::ConfigPack& config_pack,
    std::vector<Project>& projects
)
{
    if (!init_params.action.has_any_action()) {
        return true;
    }

    const ActionParams& action       = init_params.action;
    const MiscParams& misc           = init_params.misc;
    const TransformParams& transform = init_params.transform;

    if (action.model_info) {
        if (projects.empty()) {
            SPDLOG_ERROR("Cannot show info for empty projects.");
            return true;
        }

        // --info works on unrepaired model
        for (Project& project : projects) {
            Model &model = project.model();
            model.add_default_instances();
            Algorithms::Model::print_info(model);
        }
    }

    if (action.configuration_save) {
        // FIXME check for mixing the FFF / SLA parameters.
        // or better save fff_print_config vs. sla_print_config

        const std::string config_save_path =
            misc.output.has_value() ? misc.output.value() : "config.json";

        nlohmann::ordered_json config_json;
        if (std::holds_alternative<Domain::ConfigPackFDM>(config_pack)) {
            config_json = nlohmann::ordered_json(
                Domain::as_boxes(std::get<Domain::ConfigPackFDM>(config_pack))
            );
        } else if (std::holds_alternative<Domain::ConfigPackSLA>(config_pack)) {
            config_json = nlohmann::ordered_json(
                Domain::as_boxes(std::get<Domain::ConfigPackSLA>(config_pack))
            );
        } else {
            PANIC("Unexpected config type!");
        }

        boost::nowide::ofstream config_file;
        config_file.open(config_save_path, std::ios::out | std::ios::trunc);
        if (config_file.is_open()) {
            config_file << config_json.dump() << std::endl;
            config_file.close();
        } else {
            SPDLOG_ERROR("Cannot open file {} for writing", config_save_path);
            return false;
        }
    }

    if (projects.empty() && (action.export_stl || action.export_obj || action.export_3mf)) {
        SPDLOG_ERROR("Cannot export empty projects.");
        return true;
    }

    const std::string output =
        init_params.misc.output.has_value() ? init_params.misc.output.value() : "";

    if (action.export_stl) {
        for (Project& project : projects) {
            Model& model = project.model();
            model.add_default_instances();
        }

        if (!export_projects(projects, IO::STL, output)) {
            return true;
        }
    }

    if (action.export_obj) {
        for (Project& project : projects) {
            Model& model = project.model();
            model.add_default_instances();
        }

        if (!export_projects(projects, IO::OBJ, output)) {
            return true;
        }
    }

    if (action.export_3mf) {
        if (!export_projects(projects, IO::TMF, output)) {
            return true;
        }
    }

    if (action.slice || action.export_gcode || action.export_sla) {
        Domain::PrinterTechnology printer_technology = get_printer_technology(config_pack);
        if (action.export_gcode && printer_technology == Domain::PrinterTechnology::SLA) {
            boost::nowide::cerr
                << "Error: Cannot export G-code for an FFF configuration."
                << std::endl;
            return true;
        } else if (action.export_sla && printer_technology == Domain::PrinterTechnology::FFF) {
            boost::nowide::cerr
                << "error: Cannot export SLA slices for a SLA configuration."
                << std::endl;
            return true;
        }

        const Vec2crd gap{s_multiple_beds.get_bed_gap()};
        arr2::ArrangeBed bed = arr2::to_arrange_bed(get_bed_shape(config_pack), gap);
        arr2::ArrangeSettings arrange_cfg;
        arrange_cfg.set_distance_from_objects(static_cast<float>(min_object_distance(config_pack)));

        Biz::Platform::PlatformServices& platform_services =
            Biz::Platform::PlatformServices::instance();
        platform_services.set_secret_store(std::make_unique<Biz::SecretStoreDummy>());
        platform_services.set_main_thread_dispatcher(
            std::make_unique<App::Platform::StdMainThreadDispatcher>()
        );
        platform_services.set_job_manager(
            std::make_unique<Biz::Platform::JobManager::JobManager>(
                platform_services.main_thread_dispatcher()
            )
        );

        Biz::Platform::IMainThreadDispatcher& dispatcher =
            platform_services.main_thread_dispatcher();
        Domain::Workbench workbench;
        CLIThumbnailImageGenerator thumbnail_image_generator{init_params.input.input_files};
        ProjectInteractor project_interactor{workbench, dispatcher, thumbnail_image_generator};

        Preset::PresetInteractor& preset_interactor = project_interactor.preset_interactor();

        // Load new presets.
        preset_interactor.load_preset_bundle(Preset::IO::BundlePaths::make_standard_runtime());

        for (Project& project : projects) {
            Model &model = project.model();
            // If all objects have defined instances, their relative positions will be
            // honored when printing (they will be only centered, unless --dont-arrange
            // is supplied); if any object has no instances, it will get a default one
            // and all instances will be rearranged (unless --dont-arrange is supplied).
            if (!transform.dont_arrange.has_value() || !transform.dont_arrange.value()) {
                if (transform.center.has_value()) {
                    const Vec2d c = transform.center.value();
                    arrange_objects(
                        model,
                        arr2::InfiniteBed{Algorithms::Scaling::scaled(c)},
                        arrange_cfg
                    );
                } else {
                    arrange_objects(model, bed, arrange_cfg);
                }
            }

            const std::optional<std::string> slicing_errors = slice_single_model_project(
                std::move(project),
                project_interactor,
                config_pack,
                init_params.misc.output
            );
            if (slicing_errors.has_value()) {
                boost::nowide::cerr << slicing_errors.value() << std::endl;
                return true;
            }
        }

        dispatcher.close();
    }

    if (action.has_preset_updater_action()) {
        if (!misc.loglevel) {
            Slic3r::set_log_level(0);
        }
        const std::string additional_data =
            misc.output.has_value() ? misc.output.value() : std::string();

        Biz::Platform::PlatformServices& platform_services =
            Biz::Platform::PlatformServices::instance();
        platform_services.set_secret_store(std::make_unique<Biz::SecretStoreDummy>());
        platform_services.set_main_thread_dispatcher(
            std::make_unique<App::Platform::StdMainThreadDispatcher>()
        );
        platform_services.set_job_manager(
            std::make_unique<Biz::Platform::JobManager::JobManager>(
                platform_services.main_thread_dispatcher()
            )
        );

        // Potentially here we can only create standalone PresetUpdaterInteractor (without ProjectInteractor).
        // Currently every method of PresetUpdaterInteractor works only on data in filesystem and there are no listeners added in ProjectInteractor.
        Biz::Platform::IMainThreadDispatcher& dispatcher =
            platform_services.main_thread_dispatcher();
        Domain::Workbench workbench;
        CLIThumbnailImageGenerator thumbnail_image_generator{init_params.input.input_files};
        ProjectInteractor project_interactor{workbench, dispatcher, thumbnail_image_generator};

        PresetUpdaterCLI pu(project_interactor.preset_updater_interactor());
        pu.start(action, additional_data);
        while (true) {
            dispatcher.dispatch_enqueued();
            if (pu.has_result()) {
                dispatcher.close();
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    return true;
}

} // namespace Slic3r::App::CLI
