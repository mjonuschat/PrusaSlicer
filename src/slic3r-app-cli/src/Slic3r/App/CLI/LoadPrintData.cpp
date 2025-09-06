#include "Slic3r/App/CLI/LoadPrintData.hpp"

#include "Slic3r/App/CLI/ProcessActions.hpp"
#include "Slic3r/App/CLI/ProfilesSharingUtils.hpp"
#include "Slic3r/App/Init.hpp"
#include "Slic3r/Biz/Config/ConfigLegacy.hpp"
#include "Slic3r/Biz/Config/ConfigLoad.hpp"
#include "Slic3r/Biz/FileLoadingLogic.hpp"
#include "Slic3r/Biz/Preset/PresetInteractor.hpp"
#include "Slic3r/Directories.hpp"
#include "Slic3r/Domain/Model.hpp"
#include "Slic3r/Domain/Project.hpp"

#include <cstdio>
#include <fstream>
#include <string>
#include <variant>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/nowide/args.hpp>
#include <boost/nowide/iostream.hpp>

#include <nlohmann/json.hpp>

#include "libslic3r/GCode/PostProcessor.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/libslic3r.h"

namespace fs = boost::filesystem;

using namespace Slic3r;
using namespace Slic3r::Biz;

using Slic3r::Domain::Vec2d;

namespace Slic3r::App::CLI {

void merge_fdm_config_packs(
    Domain::ConfigPackFDM& target_config_pack,
    const Domain::ConfigPackFDM& source_config_pack
)
{
    target_config_pack.printer = source_config_pack.printer;
    target_config_pack.print   = source_config_pack.print;
    target_config_pack.project = source_config_pack.project;

    if (target_config_pack.tool.size() < source_config_pack.tool.size()) {
        target_config_pack.tool.resize(source_config_pack.tool.size());
    }

    for (size_t i = 0; i < source_config_pack.tool.size(); ++i) {
        target_config_pack.tool[i] = source_config_pack.tool[i];
    }

    if (target_config_pack.filament.size() < source_config_pack.filament.size()) {
        target_config_pack.filament.resize(source_config_pack.filament.size());
    }

    for (size_t i = 0; i < source_config_pack.filament.size(); ++i) {
        target_config_pack.filament[i] = source_config_pack.filament[i];
    }
}

void merge_sla_config_packs(
    Domain::ConfigPackSLA& target_config_pack,
    const Domain::ConfigPackSLA& source_config_pack
)
{
    target_config_pack.sla_printer_settings  = source_config_pack.sla_printer_settings;
    target_config_pack.sla_print_settings    = source_config_pack.sla_print_settings;
    target_config_pack.sla_material_settings = source_config_pack.sla_material_settings;
}

bool merge_config_pack(
    Domain::ConfigPack& target_config_pack,
    const Domain::ConfigPack& source_config_pack
)
{
    // Check if both ConfigPacks are the same PrinterTechnology.
    if (target_config_pack.index() != source_config_pack.index()) {
        return false; // Cannot merge different PrinterTechnology (FDM vs SLA).
    }

    if (std::holds_alternative<Domain::ConfigPackFDM>(target_config_pack)) {
        merge_fdm_config_packs(
            std::get<Domain::ConfigPackFDM>(target_config_pack),
            std::get<Domain::ConfigPackFDM>(source_config_pack)
        );
    } else if (std::holds_alternative<Domain::ConfigPackSLA>(target_config_pack)) {
        merge_sla_config_packs(
            std::get<Domain::ConfigPackSLA>(target_config_pack),
            std::get<Domain::ConfigPackSLA>(source_config_pack)
        );
    } else {
        return false;
    }

    return true;
}

std::optional<Domain::PrinterTechnology> get_printer_technology(const InitParams& init_params)
{
    for (const Domain::ConfigItem& config_item : init_params.config_overrides) {
        return config_item.get<Domain::PrinterTechnology>();
    }

    return std::nullopt;
}

Domain::PrinterTechnology get_printer_technology(const Domain::ConfigPack& config_pack)
{
    if (std::holds_alternative<Domain::ConfigPackFDM>(config_pack)) {
        return Domain::PrinterTechnology::FFF;
    } else if (std::holds_alternative<Domain::ConfigPackSLA>(config_pack)) {
        return Domain::PrinterTechnology::SLA;
    } else {
        PANIC("Unexpected config type!");
    }
}

static bool can_apply_printer_technology(
    std::optional<Domain::PrinterTechnology>& printer_technology,
    const Domain::PrinterTechnology& other_printer_technology
)
{
    if (!printer_technology.has_value()) {
        printer_technology = other_printer_technology;
        return true;
    }

    if (printer_technology.value() != other_printer_technology) {
        boost::nowide::cerr << "Mixing configurations for FFF and SLA technologies" << std::endl;
        return false;
    }

    return true;
}

static std::optional<Domain::ConfigPack> load_config_from_file(const std::string& file_path)
{
    if (!boost::filesystem::exists(file_path)) {
        return std::nullopt;
    }

    try {
        if (boost::algorithm::iends_with(file_path, ".json")) {
            // Load JSON config.
            std::ifstream file(file_path);
            if (!file.is_open()) {
                boost::nowide::cerr
                    << "Error while reading JSON config from "
                    << file_path
                    << std::endl;
                return std::nullopt;
            }

            nlohmann::ordered_json json_config;
            file >> json_config;

            if (auto result = Config::load(json_config); result) {
                return result.value().config;
            } else {
                boost::nowide::cerr
                    << "Error while loading JSON config from "
                    << file_path
                    << std::endl;
            }
        } else {
            // Load legacy INI config.
            return load_config_from_legacy_file(file_path);
        }
    } catch (std::exception& ex) {
        boost::nowide::cerr
            << "Error while reading config file \""
            << file_path
            << "\": "
            << ex.what()
            << std::endl;
    }

    return std::nullopt;
}

static bool load_print_config(
    Domain::ConfigPack& config_pack,
    std::optional<Domain::PrinterTechnology>& printer_technology,
    const InitParams& init_params
)
{
    if (!init_params.input.config_files.empty()) {
        // Load config files supplied via --load.
        for (const std::string& file_path : init_params.input.config_files) {
            if (!boost::filesystem::exists(file_path)) {
                if (init_params.misc.ignore_nonexistent_config.has_value()
                    && init_params.misc.ignore_nonexistent_config.value())
                {
                    continue;
                } else {
                    boost::nowide::cerr << "No such file: " << file_path << std::endl;
                    return false;
                }
            }

            std::optional<Domain::ConfigPack> loaded_config = load_config_from_file(file_path);
            if (!loaded_config.has_value()) {
                return false;
            }

            if (!can_apply_printer_technology(
                    printer_technology,
                    get_printer_technology(loaded_config.value())
                ))
            {
                return false;
            }

            merge_config_pack(config_pack, loaded_config.value());
        }
    }

    // Then apply other options from full print config if any is provided by profiles set.
    if (has_full_config_from_profiles(init_params)) {
        Domain::ConfigPack loaded_config;
        // Load config from profiles set.
        std::string errors = load_full_print_config(
            init_params.input.print_profile_preset.value(),
            init_params.input.material_profile_presets,
            init_params.input.tool_profile_presets,
            init_params.input.printer_profile_preset.value(),
            loaded_config,
            printer_technology
        );

        if (!errors.empty()) {
            boost::nowide::cerr
                << "Error while loading config from profiles: "
                << errors
                << std::endl;
            return false;
        }

        if (!can_apply_printer_technology(
                printer_technology,
                get_printer_technology(loaded_config)
            ))
        {
            return false;
        }

        merge_config_pack(config_pack, loaded_config);
    }

    return true;
}

static bool process_input_files(
    std::vector<Domain::Model>& models,
    Domain::ConfigPack& config_pack,
    std::optional<Domain::PrinterTechnology>& printer_technology,
    InitParams& init_params
)
{
    for (const std::string& file : init_params.input.input_files) {
        if (boost::starts_with(file, "prusaslicer://")) {
            continue;
        }

        if (!boost::filesystem::exists(file)) {
            boost::nowide::cerr << "No such file: " << file << std::endl;
            return false;
        }

        Domain::Model model;
        try {
            if (has_full_config_from_profiles(init_params)
                || !FileLoadingLogic::is_project_file(file))
            {
                // We have a full bunch of options from profiles set, so just load a geometry.
                if (tl::expected<Domain::Model, std::string> model_data =
                        FileLoadingLogic::read_model_from_file(file, nullptr);
                    !model_data)
                {
                    boost::nowide::cerr << "Error: " + model_data.error() << std::endl;
                } else {
                    model = std::move(model_data.value());
                }
            } else {
                assert(FileLoadingLogic::is_project_file(file));

                Domain::Workbench workbench;
                Scene::SceneInteractor scene_interactor{workbench};
                Preset::PresetInteractor preset_interactor{workbench, scene_interactor};

                // Load new presets.
                fs::path preset_bundle_dir = fs::path{Slic3r::resources_dir()} / "presets";
                fs::path config_dir        = fs::path{Slic3r::data_dir()} / "configs";
                preset_interactor.load_preset_bundle(
                    preset_bundle_dir.string(),
                    config_dir.string()
                );

                // Load model and configuration from the file.
                Domain::Project loaded_project = Biz::FileLoadingLogic::load_file_as_project(
                    file,
                    workbench.preset_bundle(),
                    nullptr
                );

                if (loaded_project.config_containers().empty()) {
                    loaded_project.config_containers().emplace_back(
                        std::make_unique<Domain::ConfigContainer>()
                    );
                    preset_interactor.initialize_config_container(
                        *loaded_project.config_containers().back()
                    );
                }

                // TODO: For now, we always use the first ConfigContainer.
                Domain::ConfigPack loaded_config =
                    loaded_project.config_containers().front()->print_config();

                if (!can_apply_printer_technology(
                        printer_technology,
                        get_printer_technology(loaded_config)
                    ))
                {
                    return false;
                }

                model = std::move(loaded_project.model());
                // Config is applied with config_pack loaded before.
                merge_config_pack(config_pack, loaded_config);
            }

            // If model for slicing is loaded from 3mf file, then its geometry has to be used and arrange couldn't be apply for this model.
            if (FileLoadingLogic::is_project_file(file)
                && (!init_params.transform.dont_arrange.has_value()
                    || !init_params.transform.dont_arrange.value()))
            {
                // So, check a state of "dont_arrange" parameter and set it to true, if its value is false.
                init_params.transform.dont_arrange = true;
            }
        } catch (std::exception& e) {
            boost::nowide::cerr << file << ": " << e.what() << std::endl;
            return false;
        }

        if (model.objects.empty()) {
            boost::nowide::cerr << "Error: file is empty: " << file << std::endl;
            continue;
        }

        models.push_back(model);
    }

    return true;
}

static bool finalize_print_config(
    Domain::ConfigPack& config_pack,
    std::optional<Domain::PrinterTechnology>& printer_technology,
    const InitParams& init_params
)
{
    if (!printer_technology.has_value()) {
        printer_technology = init_params.action == ActionType::ExportSLA ?
            Domain::PrinterTechnology::SLA :
            Domain::PrinterTechnology::FFF;
    }

    if (std::holds_alternative<Domain::ConfigPackFDM>(config_pack)
        && printer_technology.value() != Domain::PrinterTechnology::FFF)
    {
        config_pack = Domain::ConfigPackFDM{};
    } else if (std::holds_alternative<Domain::ConfigPackSLA>(config_pack)
               && printer_technology.value() != Domain::PrinterTechnology::SLA)
    {
        config_pack = Domain::ConfigPackSLA{};
    }

    if (std::holds_alternative<Domain::ConfigPackFDM>(config_pack)) {
        Domain::ConfigPackFDM& config_pack_fdm = std::get<Domain::ConfigPackFDM>(config_pack);
        for (const Domain::ConfigItem& config_item_override : init_params.config_overrides) {
            if (!std::holds_alternative<Domain::FDMConfigLocation>(
                    config_item_override.def().location
                ))
            {
                boost::nowide::cerr
                    << "Error: PrinterTechnology::FFF doesn't contains configuration key: "
                        + config_item_override.def().name
                    << std::endl;
                continue;
            }

            const Domain::FDMConfigLocation& location =
                std::get<Domain::FDMConfigLocation>(config_item_override.def().location);
            switch (location) {
            case Domain::FDMConfigLocation::Printer:
                config_pack_fdm.printer.items.opt(config_item_override.def().name)
                    .set(config_item_override.value());
                break;
            case Domain::FDMConfigLocation::Print:
                config_pack_fdm.print.items.opt(config_item_override.def().name)
                    .set(config_item_override.value());
                break;
            case Domain::FDMConfigLocation::Tool:
                config_pack_fdm.tool.front()
                    .items.opt(config_item_override.def().name)
                    .set(config_item_override.value());
                break;
            case Domain::FDMConfigLocation::Filament:
                config_pack_fdm.filament.front()
                    .items.opt(config_item_override.def().name)
                    .set(config_item_override.value());
                break;
            default:
                PANIC("Unsupported location {}", location);
            }
        }
    } else if (std::holds_alternative<Domain::ConfigPackSLA>(config_pack)) {
        Domain::ConfigPackSLA& config_pack_sla = std::get<Domain::ConfigPackSLA>(config_pack);
        for (const Domain::ConfigItem& config_item_override : init_params.config_overrides) {
            if (!std::holds_alternative<Domain::SLAConfigLocation>(
                    config_item_override.def().location
                ))
            {
                boost::nowide::cerr
                    << "Error: PrinterTechnology::SLA doesn't contains configuration key: "
                        + config_item_override.def().name
                    << std::endl;
                continue;
            }

            const Domain::SLAConfigLocation& location =
                std::get<Domain::SLAConfigLocation>(config_item_override.def().location);
            switch (location) {
            case Domain::SLAConfigLocation::Printer:
                config_pack_sla.sla_printer_settings.items.opt(config_item_override.def().name)
                    .set(config_item_override.value());
                break;
            case Domain::SLAConfigLocation::Print:
                config_pack_sla.sla_print_settings.items.opt(config_item_override.def().name)
                    .set(config_item_override.value());
                break;
            case Domain::SLAConfigLocation::Material:
                config_pack_sla.sla_material_settings.items.opt(config_item_override.def().name)
                    .set(config_item_override.value());
                break;
            default:
                PANIC("Unsupported location {}", location);
            }
        }
    } else {
        PANIC("Unsupported ConfigPack {}", config_pack);
    }

    // TODO: Validate print configuration.

    return true;
}

bool load_print_data(
    std::vector<Domain::Model>& models,
    Domain::ConfigPack& config_pack,
    std::optional<Domain::PrinterTechnology>& printer_technology,
    InitParams& init_params
)
{
    if (!load_print_config(config_pack, printer_technology, init_params)) {
        return false;
    }

    if (!process_input_files(models, config_pack, printer_technology, init_params)) {
        return false;
    }

    if (!finalize_print_config(config_pack, printer_technology, init_params)) {
        return false;
    }

    return true;
}

bool is_needed_post_processing(const Domain::ConfigPack& config_pack)
{
    if (!std::holds_alternative<Domain::ConfigPackFDM>(config_pack)) {
        return false;
    }

    const Domain::ConfigPackFDM& config_pack_fdm = std::get<Domain::ConfigPackFDM>(config_pack);
    if (const Domain::ConstFindResult result = config_pack_fdm.print.find("post_process");
        result.item != nullptr)
    {
        const std::vector<std::string>& post_process = result.item->get<std::vector<std::string>>();
        if (!post_process.empty()) {
            boost::nowide::cout
                << "\nA post-processing script has been detected in the config data:\n\n";
            for (const std::string& s : post_process) {
                boost::nowide::cout << "> " << s << "\n";
            }

            boost::nowide::cout << "\nContinue(Y/N) ? ";

            char in = 0;
            boost::nowide::cin >> in;
            if (in != 'Y' && in != 'y') {
                return true;
            }
        }
    }

    return false;
}

} // namespace Slic3r::App::CLI
