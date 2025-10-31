#include "Slic3r/App/CLI/ProfilesSharingUtils.hpp"

#include "Slic3r/Biz/Preset/PresetInteractor.hpp"
#include "Slic3r/Directories.hpp"
#include "Slic3r/Domain/ConfigPack.hpp"
#include "Slic3r/Domain/Preset/Bundle.hpp"
#include "Slic3r/Domain/PrinterTechnology.hpp"
#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/Domain/Workbench.hpp"

#include <ranges>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <spdlog/spdlog.h>

#include "libslic3r/BuildVolume.hpp"
#include "libslic3r/Utils/JsonUtils.hpp"
#include "libslic3r/format.hpp"

using Slic3r::Domain::PrinterTechnology;
using Slic3r::Domain::Vec2d;
using Slic3r::Domain::Vec2ds;

using namespace Slic3r;
using namespace Slic3r::Biz;

namespace fs = boost::filesystem;
namespace pt = boost::property_tree;

namespace Slic3r::App::CLI {

static bool load_preset_bundle_from_datadir(Slic3r::Domain::Preset::Bundle& preset_bundle)
{
    try {
        const std::string& data_dir = Slic3r::data_dir();
        if (data_dir.empty()) {
            SPDLOG_ERROR("Configuration wasn't found. Check your 'datadir' value.");
            return false;
        }

        Domain::Workbench workbench;
        Scene::SceneInteractor scene_interactor{workbench};
        Preset::PresetInteractor preset_interactor(workbench, scene_interactor);

        fs::path preset_bundle_dir = fs::path{resources_dir()} / "presets";
        fs::path config_dir        = fs::path{data_dir} / "configs";
        preset_interactor.load_preset_bundle(preset_bundle_dir.string(), config_dir.string());

        preset_bundle = std::move(workbench.preset_bundle());
    } catch (const std::exception& ex) {
        SPDLOG_ERROR("{}", ex.what());
        return false;
    }

    return true;
}

static void add_profile_node(
    pt::ptree& printer_profiles_node,
    const Domain::Preset::EvaluatedPrinterPreset& evaluated_preset
)
{
    pt::ptree profile_node;

    const Domain::ConfigBox& config = evaluated_preset.preset.config_box();
    const size_t extruders_cnt = evaluated_preset.technology() == Domain::PrinterTechnology::SLA ?
        0 :
        evaluated_preset.hw_config.tool_count;

    profile_node.put("name", evaluated_preset.hw_config.name);
    if (extruders_cnt > 0) {
        profile_node.put("extruders_cnt", extruders_cnt);
    }

    const double max_print_height = config.items.opt("max_print_height").get<double>();
    const Vec2ds& bed_shape       = config.items.opt("bed_shape").get<Vec2ds>();

    BuildVolume build_volume = BuildVolume{bed_shape, max_print_height};
    BoundingBoxf bb          = build_volume.bounding_volume2d();

    Vec2d origin_pt;
    if (build_volume.type() == BuildVolume::Type::Circle) {
        origin_pt = build_volume.bed_center();
    } else {
        origin_pt = to_2d(-1 * build_volume.bounding_volume().min);
    }

    std::string origin = Slic3r::format(
        "[%1%, %2%]",
        Domain::fuzzy_compare(origin_pt.x(), 0.) ? 0 : origin_pt.x(),
        Domain::fuzzy_compare(origin_pt.y(), 0.) ? 0 : origin_pt.y()
    );

    pt::ptree bed_node;
    bed_node.put("type", build_volume.type_name());
    bed_node.put("width", bb.max.x() - bb.min.x());
    bed_node.put("height", bb.max.y() - bb.min.y());
    bed_node.put("origin", origin);
    bed_node.put("max_print_height", max_print_height);

    profile_node.add_child("bed", bed_node);

    printer_profiles_node.push_back(std::make_pair("", profile_node));
}

static void add_printer_models(
    pt::ptree& printer_models_node,
    const Domain::Preset::VendorBundle& vendor_bundle,
    const Domain::Preset::EvaluatedPrinterPresets& evaluated_presets,
    const std::optional<Slic3r::Domain::PrinterTechnology>& printer_technology_filter
)
{
    std::map<
        std::string,
        std::vector<std::reference_wrapper<const Domain::Preset::EvaluatedPrinterPreset>>>
        grouped_printer_presets;
    for (const std::vector<Domain::Preset::EvaluatedPrinterPreset>& evaluated_presets_items :
         evaluated_presets | std::views::values)
    {
        for (const Domain::Preset::EvaluatedPrinterPreset& evaluated_preset :
             evaluated_presets_items)
        {
            if (printer_technology_filter.has_value()
                && evaluated_preset.technology() != printer_technology_filter.value())
            {
                continue;
            }

            if (evaluated_preset.hw_config.vendor_id != vendor_bundle.vendor_data.info.id) {
                continue;
            }

            grouped_printer_presets[evaluated_preset.preset.id].emplace_back(
                std::cref(evaluated_preset)
            );
        }
    }

    for (const auto& [preset_id, printer_presets] : grouped_printer_presets) {
        const Domain::Preset::EvaluatedPrinterPreset& first_printer_presets =
            printer_presets.front().get();

        pt::ptree variants_node;
        pt::ptree printer_profiles_node;
        pt::ptree user_printer_profiles_node;

        if (first_printer_presets.technology() == PrinterTechnology::SLA) {
            add_profile_node(printer_profiles_node, first_printer_presets);
            if (printer_profiles_node.empty() && user_printer_profiles_node.empty()) {
                continue;
            }
        } else {
            assert(first_printer_presets.technology() == PrinterTechnology::FFF);

            for (const std::reference_wrapper<const Domain::Preset::EvaluatedPrinterPreset>&
                     printer_preset_ref : printer_presets)
            {
                const Domain::Preset::EvaluatedPrinterPreset& printer_preset =
                    printer_preset_ref.get();

                printer_profiles_node.clear();
                user_printer_profiles_node.clear();

                add_profile_node(printer_profiles_node, printer_preset);
                if (printer_profiles_node.empty() && user_printer_profiles_node.empty()) {
                    continue;
                }

                pt::ptree variant_node;
                variant_node.put("id", printer_preset.hw_config.id);
                variant_node.put("name", printer_preset.hw_config.name);
                variant_node.add_child("printer_profiles", printer_profiles_node);
                if (!user_printer_profiles_node.empty()) {
                    variant_node.add_child("user_printer_profiles", user_printer_profiles_node);
                }

                variants_node.push_back(std::make_pair("", variant_node));
            }

            if (variants_node.empty()) {
                continue;
            }
        }

        pt::ptree data_node;
        data_node.put("id", first_printer_presets.preset.id);
        data_node.put("name", first_printer_presets.preset.name);
        data_node.put(
            "technology",
            first_printer_presets.technology() == PrinterTechnology::FFF ? "FFF" : "SLA"
        );

        if (!variants_node.empty())
            data_node.add_child("variants", variants_node);
        else {
            data_node.add_child("printer_profiles", printer_profiles_node);
            if (!user_printer_profiles_node.empty()) {
                data_node.add_child("user_printer_profiles", user_printer_profiles_node);
            }
        }

        data_node.put("vendor_name", vendor_bundle.vendor_data.info.name);
        data_node.put("vendor_id", vendor_bundle.vendor_data.info.id);

        printer_models_node.push_back(std::make_pair("", data_node));
    }
}

/*
static void add_undef_printer_models(pt::ptree& vendor_node,
                                     PrinterTechnology printer_technology,
                                     const PrinterPresetCollection& printer_presets)
{
    for (auto pt : { PrinterTechnology::FFF, PrinterTechnology::SLA }) {
        if (printer_technology != ptUnknown && printer_technology != pt)
            continue;

        pt::ptree printer_profiles_node;
        for (const Preset& preset : printer_presets) {
            if (!preset.is_visible || preset.printer_technology() != pt ||
                preset.vendor || printer_presets.get_preset_parent(preset))
                continue;

            add_profile_node(printer_profiles_node, preset);
        }

        if (!printer_profiles_node.empty()) {
            pt::ptree data_node;
            data_node.put("id", "");
            data_node.put("technology", pt == PrinterTechnology::FFF ? "FFF" : "SLA");
            data_node.add_child("printer_profiles", printer_profiles_node);
            data_node.put("vendor_name", "");
            data_node.put("vendor_id", "");

            vendor_node.push_back(std::make_pair("", data_node));
        }
    }
}
*/

std::string get_json_printer_models(
    std::optional<Domain::PrinterTechnology> printer_technology_filter
)
{
    Domain::Preset::Bundle preset_bundle;
    if (!load_preset_bundle_from_datadir(preset_bundle)) {
        return "";
    }

    pt::ptree printer_models_node;
    for (const Domain::Preset::VendorBundle& vendor_bundle :
         preset_bundle.vendor_bundles | std::views::values)
    {
        add_printer_models(
            printer_models_node,
            vendor_bundle,
            preset_bundle.evaluated_presets,
            printer_technology_filter
        );
    }

    // TODO: Add printers with no vendor information.
    // add_undef_printer_models(vendor_node, printer_technology, preset_bundle.printers);

    pt::ptree root;
    root.add_child("printer_models", printer_models_node);

    // Serialize the tree into JSON and return it.
    return write_json_with_post_process(root);
}

static std::string get_installed_print_tool_filament_profiles(
    const Domain::Preset::Bundle& preset_bundle,
    const Domain::Preset::EvaluatedPrinterPreset& printer_preset
)
{
    const Domain::PrinterTechnology printer_technology = printer_preset.technology();
    const std::string material_node_name = (printer_technology == Domain::PrinterTechnology::FFF) ?
        "filament_profiles" :
        "sla_material_profiles";

    pt::ptree print_profiles;
    pt::ptree user_print_profiles;

    for (const Domain::Preset::EvaluatedPrintPreset& print_preset : printer_preset.prints) {
        pt::ptree materials_profile_node;
        pt::ptree user_materials_profile_node;
        for (const Domain::Preset::SingleToolEvaluatedMaterialPresets& material_presets :
             print_preset.materials)
        {
            for (const Domain::Preset::EvaluatedMaterialPreset& material_preset : material_presets)
            {
                pt::ptree material_node;
                material_node.put("", material_preset.preset.name);

                // TODO: Add support for user presets.
                if (false) {
                    user_materials_profile_node.push_back(std::make_pair("", material_node));
                } else {
                    materials_profile_node.push_back(std::make_pair("", material_node));
                }
            }
        }

        pt::ptree tool_prints_profile_node;
        pt::ptree user_tool_prints_profile_node;
        for (const Domain::Preset::SingleToolEvaluatedToolPrintPresets& tool_presets :
             print_preset.tools)
        {
            for (const Domain::Preset::EvaluatedToolPrintPreset& tool_preset : tool_presets) {
                pt::ptree tool_print_node;
                tool_print_node.put("", tool_preset.preset.name);

                // TODO: Add support for user presets.
                if (false) {
                    user_tool_prints_profile_node.push_back(std::make_pair("", tool_print_node));
                } else {
                    tool_prints_profile_node.push_back(std::make_pair("", tool_print_node));
                }
            }
        }

        pt::ptree print_profile_node;
        print_profile_node.put("name", print_preset.preset.name);

        if (printer_technology == Domain::PrinterTechnology::FFF) {
            print_profile_node.add_child("tool_print_profiles", tool_prints_profile_node);
            if (!user_tool_prints_profile_node.empty()) {
                print_profile_node.add_child(
                    "user_tool_print_profiles",
                    user_tool_prints_profile_node
                );
            }
        }

        print_profile_node.add_child(material_node_name, materials_profile_node);
        if (!user_materials_profile_node.empty()) {
            print_profile_node.add_child("user_" + material_node_name, user_materials_profile_node);
        }

        // TODO: Add support for user presets.
        if (false) {
            user_print_profiles.push_back(std::make_pair("", print_profile_node));
        } else {
            print_profiles.push_back(std::make_pair("", print_profile_node));
        }
    }

    if (print_profiles.empty() && user_print_profiles.empty()) {
        return "";
    }

    pt::ptree tree;
    tree.put("printer_profile", printer_preset.preset.name);
    tree.add_child("print_profiles", print_profiles);
    if (!user_print_profiles.empty()) {
        tree.add_child("user_print_profiles", user_print_profiles);
    }

    // Serialize the tree into JSON and return it.
    return write_json_with_post_process(tree);
}

std::string get_json_print_tool_filament_profiles(const std::string& printer_profile)
{
    Domain::Preset::Bundle preset_bundle;
    if (!load_preset_bundle_from_datadir(preset_bundle)) {
        return "";
    }

    // Find a printer preset by name in evaluated presets.
    const Domain::Preset::EvaluatedPrinterPreset* printer_preset = nullptr;
    for (const std::vector<Domain::Preset::EvaluatedPrinterPreset>& evaluated_presets :
         preset_bundle.evaluated_presets | std::views::values)
    {
        const auto printer_preset_it = std::ranges::find_if(
            evaluated_presets,
            [&printer_profile](const Domain::Preset::EvaluatedPrinterPreset& preset)
            { return preset.hw_config.name == printer_profile; }
        );
        if (printer_preset_it != evaluated_presets.end()) {
            printer_preset = &*printer_preset_it;
            break;
        }
    }

    if (printer_preset != nullptr) {
        return get_installed_print_tool_filament_profiles(preset_bundle, *printer_preset);
    }

    return "";
}

// Find a printer preset by name in evaluated presets.
const Domain::Preset::EvaluatedPrinterPreset* find_printer_preset_by_name(
    const Domain::Preset::Bundle& preset_bundle,
    const std::string& printer_preset_name
)
{
    for (const std::vector<Domain::Preset::EvaluatedPrinterPreset>& printer_presets :
         preset_bundle.evaluated_presets | std::views::values)
    {
        for (const Domain::Preset::EvaluatedPrinterPreset& printer_preset : printer_presets) {
            if (printer_preset.hw_config.name == printer_preset_name) {
                return &printer_preset;
            }
        }
    }

    return nullptr;
}

// Find a print preset by name in the compatible print presets.
const Domain::Preset::EvaluatedPrintPreset* find_print_preset_by_name(
    const Domain::Preset::EvaluatedPrinterPreset& printer_preset,
    const std::string& print_preset_name
)
{
    for (const Domain::Preset::EvaluatedPrintPreset& print_preset : printer_preset.prints) {
        if (print_preset.preset.name == print_preset_name) {
            return &print_preset;
        }
    }

    return nullptr;
}

// Find a tool preset by name in the compatible print presets.
const Domain::Preset::EvaluatedToolPrintPreset* find_tool_preset_by_name(
    const Domain::Preset::EvaluatedPrintPreset& print_preset,
    const std::string& tool_preset_name
)
{
    if (print_preset.tools.empty()) {
        return nullptr;
    }

    // TODO: For now support only single tool printers.
    assert(print_preset.tools.size() == 1);
    const Domain::Preset::SingleToolEvaluatedToolPrintPresets& tool_presets =
        print_preset.tools.front();
    for (const Domain::Preset::EvaluatedToolPrintPreset& tool_preset : tool_presets) {
        if (tool_preset.preset.name == tool_preset_name) {
            return &tool_preset;
        }
    }

    return nullptr;
}

// Find a material preset by name in the compatible print presets.
const Domain::Preset::EvaluatedMaterialPreset* find_material_preset_by_name(
    const Domain::Preset::EvaluatedPrintPreset& print_preset,
    const std::string& material_preset_name
)
{
    if (print_preset.materials.empty()) {
        return nullptr;
    }

    // TODO: For now support only single tool printers.
    assert(print_preset.materials.size() == 1);
    const Domain::Preset::SingleToolEvaluatedMaterialPresets& material_presets =
        print_preset.materials.front();
    for (const Domain::Preset::EvaluatedMaterialPreset& material_preset : material_presets) {
        if (material_preset.preset.name == material_preset_name) {
            return &material_preset;
        }
    }

    return nullptr;
}

// Helper function for load full config from installed presets by profile names
std::string load_full_print_config(
    const std::string& print_preset_name,
    const std::vector<std::string>& material_preset_names_in,
    const std::vector<std::string>& tool_preset_names_in,
    const std::string& printer_preset_name,
    Domain::ConfigPack& config,
    std::optional<Domain::PrinterTechnology> printer_technology
)
{
    // Check entered profile names.
    if (print_preset_name.empty()
        || material_preset_names_in.empty()
        || printer_preset_name.empty())
    {
        return "Request is not completed. All of Print/Material/Printer profiles have to be entered";
    }

    // Check preset bundle.
    Domain::Preset::Bundle preset_bundle;
    if (!load_preset_bundle_from_datadir(preset_bundle)) {
        return Slic3r::format("Failed to load data from the datadir '%1%'.", data_dir());
    }

    // check existance of required profiles

    std::string errors;

    const Domain::Preset::EvaluatedPrinterPreset* printer_preset =
        find_printer_preset_by_name(preset_bundle, printer_preset_name);
    if (printer_preset == nullptr) {
        errors += "\n" + Slic3r::format("Printer profile '%1%' wasn't found.", printer_preset_name);
    } else if (!printer_technology.has_value()) {
        printer_technology = printer_preset->technology();
    } else if (printer_technology != printer_preset->technology()) {
        errors +=
            "\n"
            + std::string(
                "Printer technology of the selected printer preset is differs with required printer technology"
            );
    }

    const Domain::Preset::EvaluatedPrintPreset* print_preset =
        find_print_preset_by_name(*printer_preset, print_preset_name);
    if (print_preset == nullptr) {
        errors += "\n" + Slic3r::format("Print profile '%1%' wasn't found.", print_preset_name);
    }

    auto check_material =
        [&print_preset](const std::string& material_preset_name, std::string& errors) -> void
    {
        const Domain::Preset::EvaluatedMaterialPreset* material_preset =
            find_material_preset_by_name(*print_preset, material_preset_name);
        if (material_preset == nullptr) {
            errors +=
                "\n" + Slic3r::format("Material profile '%1%' wasn't found.", material_preset_name);
        }
    };

    check_material(material_preset_names_in.front(), errors);
    if (material_preset_names_in.size() > 1) {
        for (size_t idx = 1; idx < material_preset_names_in.size(); idx++) {
            if (material_preset_names_in[idx] != material_preset_names_in.front()) {
                check_material(material_preset_names_in[idx], errors);
            }
        }
    }

    auto check_tool =
        [&print_preset](const std::string& tool_preset_name, std::string& errors) -> void
    {
        const Domain::Preset::EvaluatedToolPrintPreset* tool_print_preset =
            find_tool_preset_by_name(*print_preset, tool_preset_name);
        if (tool_print_preset == nullptr) {
            errors +=
                "\n" + Slic3r::format("Tool print profile '%1%' wasn't found.", tool_preset_name);
        }
    };

    check_tool(tool_preset_names_in.front(), errors);
    if (tool_preset_names_in.size() > 1) {
        for (size_t idx = 1; idx < tool_preset_names_in.size(); idx++) {
            if (tool_preset_names_in[idx] != tool_preset_names_in.front()) {
                check_tool(tool_preset_names_in[idx], errors);
            }
        }
    }

    if (!errors.empty()) {
        return errors;
    }

    // Adjust the material preset list and the tool print preset list based on printer technology.
    std::vector<std::string> material_preset_names = material_preset_names_in;
    std::vector<std::string> tool_preset_names     = tool_preset_names_in;
    if (printer_technology == Domain::PrinterTechnology::SLA) {
        if (material_preset_names.size() > 1) {
            // For SLA, adjust the material count to be equal to one.
            SPDLOG_WARN(
                "Note: More than one sla material profiles were entered. Extras material profiles will be ignored."
            );
            material_preset_names.resize(1);
        }

        if (tool_preset_names.size() > 1) {
            // For SLA, adjust the tool print count to be equal to one.
            SPDLOG_WARN(
                "Note: More than one sla tool print profiles were entered. Extras tool print profiles will be ignored."
            );
            tool_preset_names.resize(1);
        }
    }

    if (printer_technology == Domain::PrinterTechnology::FFF) {
        // For FDM, adjust the material count to match the extruder count.
        const int extruders_count = printer_preset->hw_config.tool_count;
        if (extruders_count > static_cast<int>(material_preset_names.size())) {
            SPDLOG_WARN(
                "Note: Less than needed filament profiles were entered. Missed filament profiles will be filled with first material."
            );
            material_preset_names.reserve(extruders_count);
            for (int i = extruders_count - static_cast<int>(material_preset_names.size()); i > 0;
                 --i)
            {
                material_preset_names.push_back(material_preset_names.front());
            }
        } else if (extruders_count < static_cast<int>(material_preset_names.size())) {
            SPDLOG_WARN(
                "Note: More than needed filament profiles were entered. Extra filament profiles will be ignored."
            );
            material_preset_names.resize(extruders_count);
        }

        // For FDM, adjust the tool print count to match the extruder count.
        if (extruders_count > static_cast<int>(tool_preset_names.size())) {
            SPDLOG_WARN(
                "Note: Less than needed tool print profiles were entered. Missed tool print profiles will be filled with first tool print."
            );
            tool_preset_names.reserve(extruders_count);
            for (int i = extruders_count - static_cast<int>(tool_preset_names.size()); i > 0; --i) {
                tool_preset_names.push_back(tool_preset_names.front());
            }
        } else if (extruders_count < static_cast<int>(tool_preset_names.size())) {
            SPDLOG_WARN(
                "Note: More than needed tool print profiles were entered. Extra tool print profiles will be ignored."
            );
            tool_preset_names.resize(extruders_count);
        }
    }

    std::vector<Domain::Preset::EvaluatedToolPrintPreset::Preset> tool_presets;
    for (const std::string& tool_preset_name : tool_preset_names) {
        const Domain::Preset::EvaluatedToolPrintPreset* tool_preset =
            find_tool_preset_by_name(*print_preset, tool_preset_name);
        assert(tool_preset != nullptr);
        tool_presets.emplace_back(tool_preset->preset);
    }

    std::vector<Domain::Preset::EvaluatedMaterialPreset::Preset> material_presets;
    for (const std::string& material_preset_name : material_preset_names) {
        const Domain::Preset::EvaluatedMaterialPreset* material_preset =
            find_material_preset_by_name(*print_preset, material_preset_name);
        assert(material_preset != nullptr);
        material_presets.emplace_back(material_preset->preset);
    }

    if (printer_technology == Domain::PrinterTechnology::FFF) {
        Domain::ConfigPackFDM config_pack_fdm;
        config_pack_fdm.printer = std::get<Domain::PrinterSettings>(printer_preset->preset.values);
        config_pack_fdm.print   = std::get<Domain::PrintSettings>(print_preset->preset.values);

        config_pack_fdm.tool.resize(tool_presets.size());
        for (size_t i = 0; i < tool_presets.size(); ++i) {
            config_pack_fdm.tool[i] = std::get<Domain::ToolPrintSettings>(tool_presets[i].values);
        }

        config_pack_fdm.filament.resize(material_presets.size());
        for (size_t i = 0; i < material_presets.size(); ++i) {
            config_pack_fdm.filament[i] =
                std::get<Domain::FilamentSettings>(material_presets[i].values);
        }

        config = config_pack_fdm;
    } else {
        ASSERT(
            printer_technology == Domain::PrinterTechnology::FFF && material_presets.size() == 1
        );

        Domain::ConfigPackSLA config_pack_fdm;
        config_pack_fdm.sla_printer_settings =
            std::get<Domain::SLAPrinterSettings>(printer_preset->preset.values);
        config_pack_fdm.sla_print_settings =
            std::get<Domain::SLAPrintSettings>(print_preset->preset.values);
        config_pack_fdm.sla_material_settings =
            std::get<Domain::SLAMaterialSettings>(material_presets.front().values);

        config = config_pack_fdm;
    }

    return "";
}

} // namespace Slic3r::App::CLI
