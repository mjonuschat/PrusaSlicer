#include "Slic3r/Biz/Config/ConfigLoad.hpp"
#include <nlohmann/json.hpp>
#include <tl/expected.hpp>
#include "Slic3r/Biz/Config/ConfigJson.hpp" // IWYU pragma: keep


namespace Slic3r::Biz::Config {

using Domain::ConfigItem;
using Domain::ConfigLocation;
using Domain::ConfigPackFDM;
using Domain::ConfigPackSLA;
using Domain::EnumValueDef;
using Domain::EnumValueDefs;
using Domain::FDMConfigLocation;
using Domain::FilamentSettings;
using Domain::get_location_name;
using Domain::overloaded;
using Domain::PrinterSettings;
using Domain::PrintSettings;
using Domain::ProjectSettings;
using Domain::SLAConfigLocation;
using Domain::SLAMaterialSettings;
using Domain::SLAPrinterSettings;
using Domain::SLAPrintSettings;
using Domain::ToolPrintSettings;
using nlohmann::ordered_json;
using Domain::Vec2d;
using Domain::Percentage;
using Domain::FloatOrPercentage;
using Domain::is_std_vector_v;

namespace {

std::optional<std::string> get_enum_issue(const ordered_json& json_value, const EnumValueDefs& enum_def)
{
    if (!json_value.is_string()) {
        return "Value is not string!";
    }
    const auto serialized_value{json_value.get<std::string>()};

    const auto def_it{std::ranges::find_if(enum_def, [&](const EnumValueDef& def) {
        return def.str_serialized == serialized_value;
    })};

    if (def_it == enum_def.end()) {
        return "Value '" + serialized_value + "' is not possible enum value!";
    }
    return std::nullopt;
}

tl::expected<std::string, std::string> parse_enum(
    const ordered_json& json_value,
    const EnumValueDefs& enum_def
)
{
    if (auto issue{get_enum_issue(json_value, enum_def)}) {
        return tl::unexpected{*issue};
    }

    return json_value.get<std::string>();
}

tl::expected<std::vector<std::string>, std::string> parse_enum_vector(
    const ordered_json& json_value,
    const EnumValueDefs& enum_def
)
{
    if (!json_value.is_array()) {
        return tl::unexpected{"Value is not an array!"};
    }
    for (const auto& value : json_value) {
        if (auto issue{get_enum_issue(value, enum_def)}) {
            return tl::unexpected{*issue};
        }
    }

    return json_value.get<std::vector<std::string>>();
}

template<typename Settings>
struct BoxLoadResult
{
    Settings settings;
    BoxIssues issues;
};

tl::expected<void, ItemParsingIssue> fill_item(ConfigItem& item, const ordered_json& json_value)
{
    const auto map_error{[&](const std::string& error) {
        return ItemParsingIssue{.type = ItemParsingIssueType::InvalidFormat, .message = error};
    }};

    return item.visit(overloaded(
        [&](Domain::EnumWrapper& enum_wrapper) {
            return parse_enum(json_value, enum_wrapper.def())
                .map([&](const std::string& value) { enum_wrapper.set_string(value); })
                .map_error(map_error);
        },
        [&](Domain::EnumVectorWrapper& enum_vector_wrapper) {
            return parse_enum_vector(json_value, enum_vector_wrapper.def())
                .map([&](const std::vector<std::string>& value) {
                    enum_vector_wrapper.set_strings(value);
                })
                .map_error(map_error);
        },
        [&](auto& box_value) -> tl::expected<void, ItemParsingIssue> {
            using ValueType = std::remove_cvref_t<decltype(box_value)>;
            return parse<ValueType>(json_value)
                .map([&](const ValueType& value) { box_value = value; })
                .map_error(map_error);
        }
    ));
}

template<typename Settings>
BoxLoadResult<Settings> load_box(const ordered_json& json)
{
    BoxLoadResult<Settings> result;

    ASSERT(json.is_object());

    BoxIssues& issues{result.issues};

    std::set<std::string> json_keys;
    for (const auto& [key, _] : json.items()) {
        json_keys.insert(key);
    }

    for (ConfigItem& item : result.settings.items) {
        const auto it{json.find(item.name())};
        if (it != json.end()) {
            fill_item(item, *it).or_else([&](const ItemParsingIssue& issue) {
                issues[item.name()] = issue;
            });
            json_keys.erase(item.name());
        } else {
            issues[item.name()] = {ItemParsingIssueType::NotFound};
        }
    }

    Domain::ConfigOverrides& overrides{result.settings.overrides};
    for (ConfigItem& item : overrides.all_items()) {
        const auto it{json.find(item.name())};
        if (it != json.end()) {
            if (!it->is_null()) {
                fill_item(item, *it) //
                    .map([&]() { overrides.enable(item.name()); })
                    .or_else([&](const ItemParsingIssue& issue) { issues[item.name()] = issue; });
            }
            json_keys.erase(item.name());
        }
    }

    for (const std::string& key : json_keys) {
        issues[key] = {ItemParsingIssueType::ExtraKey};
    }

    return result;
}

std::size_t get_most_common_size(const ordered_json& json)
{
    std::map<std::size_t, std::size_t> sizes_counts;

    for (const auto& [key, value] : json.items()) {
        if (!value.is_array()) {
            continue;
        }
        sizes_counts[value.size()]++;
    }

    if (sizes_counts.empty()) {
        return 0;
    }

    const auto it{std::ranges::max_element(sizes_counts, [](const auto& pair_a, const auto& pair_b) {
        return pair_a.second < pair_b.second;
    })};
    return it->first;
}

template<typename Settings>
struct BoxesLoadResult
{
    std::vector<Settings> settings;
    std::vector<BoxIssues> issues;
};

template<typename Settings>
BoxesLoadResult<Settings> load_boxes(const ordered_json& json)
{
    ASSERT(json.is_object());

    std::set<std::string> json_keys;
    const std::size_t boxes_count{std::max(std::size_t{1}, get_most_common_size(json))};

    std::vector<ordered_json> json_per_box(boxes_count, ordered_json::object());
    for (const auto& [key, value] : json.items()) {
        if (!value.is_array()) {
            continue;
        }
        for (std::size_t i{}; i < value.size(); ++i) {
            if (i >= json_per_box.size()) {
                continue;
            }
            json_per_box.at(i)[key] = value.at(i);
        }
    }

    BoxesLoadResult<Settings> result;
    for (const ordered_json& box_json : json_per_box) {
        const auto [settings, issues]{load_box<Settings>(box_json)};
        result.settings.push_back(settings);
        result.issues.push_back(issues);
    }
    return result;
}

bool is_object(const std::string& location_name, const ordered_json& json)
{
    return json.contains(location_name) && json[location_name].is_object();
}

bool is_empty(const std::vector<BoxIssues>& issues)
{
    for (const BoxIssues& box_issues : issues) {
        if (!box_issues.empty()) {
            return false;
        }
    }
    return true;
}
} // namespace

tl::expected<LoadResult, GlobalParsingIssue> load_fdm(const ordered_json& json)
{
    const std::string printer_location_name{get_location_name(FDMConfigLocation::Printer)};
    if (!is_object(printer_location_name, json)) {
        return tl::unexpected{GlobalParsingIssue::InvalidFDMPrinterSettings};
    }
    const auto printer_load_result{load_box<PrinterSettings>(json[printer_location_name])};

    const std::string tool_location_name{get_location_name(FDMConfigLocation::Tool)};
    if (!is_object(tool_location_name, json)) {
        return tl::unexpected{GlobalParsingIssue::InvalidFDMToolSettings};
    }
    const auto tool_load_result{load_boxes<ToolPrintSettings>(json[tool_location_name])};
    if (tool_load_result.settings.empty()) {
        return tl::unexpected{GlobalParsingIssue::InvalidFDMToolSettings};
    }

    const std::string print_location_name{get_location_name(FDMConfigLocation::Print)};
    if (!is_object(print_location_name, json)) {
        return tl::unexpected{GlobalParsingIssue::InvalidFDMPrintSettings};
    }
    const auto print_load_result{load_box<PrintSettings>(json[print_location_name])};

    const std::string filament_location_name{get_location_name(FDMConfigLocation::Filament)};
    if (!is_object(filament_location_name, json)) {
        return tl::unexpected{GlobalParsingIssue::InvalidFDMFilamentSettings};
    }
    const auto filament_load_result{load_boxes<FilamentSettings>(json[filament_location_name])};
    if (filament_load_result.settings.empty()) {
        return tl::unexpected{GlobalParsingIssue::InvalidFDMFilamentSettings};
    }
    if (filament_load_result.settings.size() != tool_load_result.settings.size()) {
        return tl::unexpected{GlobalParsingIssue::FilamentsAndToolsCountIsNotEqual};
    }

    const std::string project_location_name{get_location_name(FDMConfigLocation::Project)};
    if (!is_object(project_location_name, json)) {
        return tl::unexpected{GlobalParsingIssue::InvalidFDMProjectSettings};
    }
    const auto project_load_result{load_box<ProjectSettings>(json[project_location_name])};

    ConfigPackFDM config;
    config.printer = printer_load_result.settings;
    config.tool = tool_load_result.settings;
    config.print = print_load_result.settings;
    config.filament = filament_load_result.settings;
    config.project = project_load_result.settings;

    IssuesPerLocation issues;
    if (!printer_load_result.issues.empty()) {
        issues.insert({FDMConfigLocation::Printer, printer_load_result.issues});
    }
    if (!is_empty(tool_load_result.issues)) {
        issues.insert({FDMConfigLocation::Tool, tool_load_result.issues});
    }
    if (!print_load_result.issues.empty()) {
        issues.insert({FDMConfigLocation::Print, print_load_result.issues});
    }
    if (!is_empty(filament_load_result.issues)) {
        issues.insert({FDMConfigLocation::Filament, filament_load_result.issues});
    }
    if (!project_load_result.issues.empty()) {
        issues.insert({FDMConfigLocation::Project, project_load_result.issues});
    }

    return LoadResult{.config = std::move(config), .issues = std::move(issues)};
}

tl::expected<LoadResult, GlobalParsingIssue> load_sla(const ordered_json& json)
{
    const std::string printer_location_name{get_location_name(SLAConfigLocation::Printer)};
    if (!is_object(printer_location_name, json)) {
        return tl::unexpected{GlobalParsingIssue::InvalidSLAPrinterSettings};
    }
    const auto printer_load_result{load_box<SLAPrinterSettings>(json[printer_location_name])};

    const std::string material_location_name{get_location_name(SLAConfigLocation::Material)};
    if (!is_object(material_location_name, json)) {
        return tl::unexpected{GlobalParsingIssue::InvalidSLAMaterialSettings};
    }
    const auto material_load_result{load_box<SLAMaterialSettings>(json[material_location_name])};

    const std::string print_location_name{get_location_name(SLAConfigLocation::Print)};
    if (!is_object(print_location_name, json)) {
        return tl::unexpected{GlobalParsingIssue::InvalidSLAPrintSettings};
    }
    const auto print_load_result{load_box<SLAPrintSettings>(json[print_location_name])};

    IssuesPerLocation issues;
    if (!printer_load_result.issues.empty()) {
        issues.insert({SLAConfigLocation::Printer, printer_load_result.issues});
    }
    if (!material_load_result.issues.empty()) {
        issues.insert({SLAConfigLocation::Material, material_load_result.issues});
    }
    if (!print_load_result.issues.empty()) {
        issues.insert({SLAConfigLocation::Print, print_load_result.issues});
    }

    return LoadResult{
        .config =
            ConfigPackSLA{
                .sla_printer_settings = printer_load_result.settings,
                .sla_material_settings = material_load_result.settings,
                .sla_print_settings = print_load_result.settings,
            },
        .issues = std::move(issues)
    };
}

std::optional<std::string> parse_technology_at_location(const ordered_json& json, const std::string& location)
{
    if (!is_object(location, json)) {
        return std::nullopt;
    }
    if (!json[location].contains("printer_technology")) {
        return std::nullopt;
    }
    if (!json[location]["printer_technology"].is_string()) {
        return std::nullopt;
    }
    return json[location]["printer_technology"].get<std::string>();
}

std::optional<std::string> parse_technology(const ordered_json& json) {
    const std::string fdm_printer_location_name{get_location_name(FDMConfigLocation::Printer)};
    const std::string sla_printer_location_name{get_location_name(SLAConfigLocation::Printer)};

    if (const auto technology{parse_technology_at_location(json, fdm_printer_location_name)}) {
        if (technology == "FFF") {
            return technology;
        }
    }

    if (const auto technology{parse_technology_at_location(json, sla_printer_location_name)}) {
        if (technology == "SLA") {
            return technology;
        }
    }

    return std::nullopt;
}


tl::expected<LoadResult, GlobalParsingIssue> load(const ordered_json& json)
{
    if (!json.is_object()) {
        return tl::unexpected{GlobalParsingIssue::NotAJsonObject};
    }

    if (parse_technology(json) == "FFF") {
        return load_fdm(json);
    } else if (parse_technology(json) == "SLA") {
        return load_sla(json);
    } else {
        return tl::unexpected{GlobalParsingIssue::UnableToDeducePrinterTechnology};
    }
}
} // namespace Slic3r::Biz::Config
