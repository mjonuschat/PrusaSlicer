#include "Slic3r/App/CLI/ProcessActions.hpp"

#include "Slic3r/App/CLI/LoadPrintData.hpp"
#include "Slic3r/App/CLI/ProcessTransform.hpp"
#include "Slic3r/App/CLI/ProfilesSharingUtils.hpp"
#include "Slic3r/App/Init.hpp"
#include "Slic3r/App/Platform/StdMainThreadDispatcher.hpp"
#include "Slic3r/Biz/Algorithms/Model.hpp"
#include "Slic3r/Biz/Config/ConfigSerialize.hpp"
#include "Slic3r/Biz/Format/3mf.hpp"
#include "Slic3r/Biz/Format/STL.hpp"
#include "Slic3r/Biz/Platform/JobManager/JobManager.hpp"
#include "Slic3r/Biz/PrintHost/IPrintHostListener.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/SecretStoreDummy.hpp"
#include "Slic3r/Biz/Slicing/SlicingInteractor.hpp"
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

#include <boost/filesystem.hpp>
#include <boost/nowide/iostream.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/nowide/filesystem.hpp>

#include <nlohmann/json.hpp>

#include <spdlog/spdlog.h>

#include <arrange-wrapper/ModelArrange.hpp>

#include "libslic3r/Format/OBJ.hpp"
#include "libslic3r/IThumbnailImageGenerator.hpp"
#include "libslic3r/MultipleBeds.hpp"

#include "stb_image_resize2.h"

namespace fs = boost::filesystem;

using namespace Slic3r;
using namespace Slic3r::Biz;

using Slic3r::Domain::Vec2crd;
using Slic3r::Domain::Vec2d;
using Slic3r::Domain::Vec2ds;

namespace Slic3r::App::CLI {

// TODO: For now we use just a dummy implementation to pass all slicing steps.
class CLIThumbnailImageGenerator : public Slicing::IThumbnailImageGenerator
{
public:
    std::future<Slicing::ThumbnailImageResults> enqueue_thumbnail_requests(
        const Slicing::ThumbnailImageRequests& requests
    ) override
    {
        std::promise<Slicing::ThumbnailImageResults> promise;
        std::future<Slicing::ThumbnailImageResults> result{promise.get_future()};
        promise.set_value(Slicing::ThumbnailImageResults{});
        return result;
    }

    void handle_enqueued_requests() override {}
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

struct ExportFinishedListener : Biz::PrintHost::IPrintHostListener
{
    std::promise<std::optional<std::string>> promise_export_error;

    void on_print_host_error(size_t id, const std::string& msg) override
    {
        promise_export_error.set_value(msg);
    }

    void on_print_host_done(size_t id) override
    {
        promise_export_error.set_value(std::nullopt);
    }

    void on_print_host_progress(size_t id, int progress) override {}

    void on_print_host_cancel(size_t id) override {}

    void on_print_host_info(size_t id, const std::string& tag, const std::string& msg) override {}
};

static std::string
output_filename(const Domain::Model& model, const Domain::ConfigPack& config_pack)
{
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

    return boost::filesystem::path(Algorithms::Model::propose_export_file_name_and_path(model))
        .replace_extension(extension)
        .string();
}

std::optional<std::string> slice_single_model_project(
    Domain::Model&& model,
    ProjectInteractor& project_interactor,
    const Domain::ConfigPack& config_pack,
    const std::optional<std::string>& output_path
)
{
    // Remove all projects before slicing another one.
    const Domain::Workbench& workbench = project_interactor.workbench();
    for (const Domain::SelectionId selection_id : workbench.projects() | std::views::keys) {
        project_interactor.remove_project(selection_id);
    }

    Biz::Platform::IMainThreadDispatcher& dispatcher =
        Biz::Platform::PlatformServices::instance().main_thread_dispatcher();

    const Domain::SelectionId project_id = project_interactor.new_project();
    project_interactor.select_project(project_id);

    Domain::Project& project = project_interactor.selected_project();
    project.model()          = std::move(model);

    // Apply the provided config_pack.
    Domain::ConfigContainer& config_container       = *project.config_containers().front();
    Domain::Preset::SelectedPresetMetadata metadata = config_container.selected_preset().metadata();
    if (std::holds_alternative<Domain::ConfigPackSLA>(config_pack)) {
        metadata.hw_config.technology = Domain::PrinterTechnology::SLA;
    }

    config_container.mutable_selected_preset() =
        Domain::Preset::SelectedPreset::make(metadata, config_pack);

    project_interactor.scene_interactor().notify_listener_on_objects();

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

    const std::string dest_path =
        [&output_path, &project_interactor, &project_id, &project, &config_pack]() -> std::string
    {
        std::string dest_path = output_path.has_value() ?
            output_path.value() :
            project_interactor.get_project_name(project_id);
        if (dest_path.empty()) {
            dest_path = output_filename(project.model(), config_pack);
        }

        const std::string parent_path = boost::filesystem::path(dest_path).parent_path().string();
        if (parent_path.empty()) {
            return (fs::current_path() / dest_path).string();
        }

        return dest_path;
    }();

    if (slicing_status_update.code == Biz::Slicing::StatusCode::Finished) {
        ExportFinishedListener export_finished_listener;
        project_interactor.print_host_interactor().add_print_host_listener(
            &export_finished_listener
        );
        project_interactor.do_export(slicing_id, dest_path);

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

    boost::nowide::cout << "Slicing result exported to " << dest_path << std::endl;

    return std::nullopt;
}

static bool has_profile_sharing_action(const InitParams& init_params)
{
    return init_params.action == ActionType::QueryPrinterModels
        || init_params.action == ActionType::QueryPrintToolFilamentProfiles;
}

bool has_full_config_from_profiles(const InitParams& init_params)
{
    const InputParams& input = init_params.input;

    return !has_profile_sharing_action(init_params)
        && (input.print_profile_preset.has_value() && !input.print_profile_preset->empty()
            || !input.material_profile_presets.empty()
            || !input.tool_profile_presets.empty()
            || input.printer_profile_preset.has_value() && !input.print_profile_preset->empty());
}

bool process_profiles_sharing(const InitParams& init_params)
{
    if (!has_profile_sharing_action(init_params)) {
        return false;
    }

    std::string ret;
    if (init_params.action == ActionType::QueryPrinterModels) {
        ret = get_json_printer_models(get_printer_technology(init_params));
    } else if (init_params.action == ActionType::QueryPrintToolFilamentProfiles) {
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
    const Domain::Model& model,
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

    auto proposed_path =
        boost::filesystem::path(Algorithms::Model::propose_export_file_name_and_path(model, ext));
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

static bool export_models(
    std::vector<Domain::Model>& models,
    IO::ExportFormat format,
    const std::string& cmdline_param
)
{
    for (Domain::Model& model : models) {
        const std::string path = output_filepath(model, format, cmdline_param);
        bool success           = false;
        switch (format) {
        case IO::OBJ: {
            success = Slic3r::store_obj(path.c_str(), &model);
            break;
        }
        case IO::STL: {
            success = store_stl(path, Algorithms::Model::flatten_to_mesh(model), true);
            break;
        }
        case IO::TMF: {
            try {
                Domain::Project project;
                project.model() = std::move(model);
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

static Domain::Image resize_and_crop(
    const std::vector<unsigned char>& data,
    const int width,
    const int height,
    const int width_new,
    const int height_new
)
{
    const float scale_x     = float(width_new) / width;
    const float scale_y     = float(height_new) / height;
    const float scale       = std::max(scale_x, scale_y); // Choose the larger scale to fill the box
    const int resized_width = int(width * scale);
    const int resized_height = int(height * scale);

    std::vector<unsigned char> resized_rgba(resized_width * resized_height * 4);
    stbir_resize_uint8_linear(
        data.data(),
        width,
        height,
        4 * width,
        resized_rgba.data(),
        resized_width,
        resized_height,
        4 * resized_width,
        STBIR_RGBA
    );

    Domain::Image th(Domain::PixelFormat::RGBA8, width_new, height_new);

    const int crop_x = (resized_width - width_new) / 2;
    const int crop_y = (resized_height - height_new) / 2;

    for (int y = 0; y < height_new; ++y) {
        std::memcpy(
            th.pixels.data() + y * width_new * 4,
            resized_rgba.data() + ((y + crop_y) * resized_width + crop_x) * 4,
            width_new * 4
        );
    }

    return th;
}

bool process_actions(
    const InitParams& init_params,
    const Domain::ConfigPack& config_pack,
    std::vector<Domain::Model>& models
)
{
    if (!init_params.action.has_value()) {
        return true;
    }

    const ActionType action          = init_params.action.value();
    const MiscParams misc            = init_params.misc;
    const TransformParams& transform = init_params.transform;

    if (action == ActionType::ModelInfo) {
        if (models.empty()) {
            SPDLOG_ERROR("Cannot show info for empty models.");
            return true;
        }

        // --info works on unrepaired model
        for (Domain::Model& model : models) {
            model.add_default_instances();
            Algorithms::Model::print_info(model);
        }
    }

    if (action == ActionType::ConfigurationSave) {
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

    if (models.empty()
        && (action == ActionType::ExportSTL
            || action == ActionType::ExportOBJ
            || action == ActionType::Export3MF))
    {
        SPDLOG_ERROR("Cannot export empty models.");
        return true;
    }

    const std::string output =
        init_params.misc.output.has_value() ? init_params.misc.output.value() : "";

    if (action == ActionType::ExportSTL) {
        for (Domain::Model& model : models) {
            model.add_default_instances();
        }

        if (!export_models(models, IO::STL, output)) {
            return true;
        }
    }

    if (action == ActionType::ExportOBJ) {
        for (Domain::Model& model : models) {
            model.add_default_instances();
        }

        if (!export_models(models, IO::OBJ, output)) {
            return true;
        }
    }

    if (action == ActionType::Export3MF) {
        if (!export_models(models, IO::TMF, output)) {
            return true;
        }
    }

    if (action == ActionType::Slice
        || action == ActionType::ExportGCode
        || action == ActionType::ExportSLA)
    {
        Domain::PrinterTechnology printer_technology = get_printer_technology(config_pack);
        if (action == ActionType::ExportGCode
            && printer_technology == Domain::PrinterTechnology::SLA)
        {
            boost::nowide::cerr
                << "Error: Cannot export G-code for an FFF configuration."
                << std::endl;
            return true;
        } else if (action == ActionType::ExportSLA
                   && printer_technology == Domain::PrinterTechnology::FFF)
        {
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
        CLIThumbnailImageGenerator thumbnail_image_generator;
        ProjectInteractor project_interactor{workbench, dispatcher, thumbnail_image_generator};

        Preset::PresetInteractor& preset_interactor = project_interactor.preset_interactor();

        // Load new presets.
        fs::path preset_bundle_dir = fs::path{Slic3r::resources_dir()} / "presets";
        fs::path config_dir        = fs::path{Slic3r::data_dir()} / "configs";
        preset_interactor.load_preset_bundle(preset_bundle_dir.string(), config_dir.string());

        for (Domain::Model& model : models) {
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
                std::move(model),
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

    return true;
}

} // namespace Slic3r::App::CLI
