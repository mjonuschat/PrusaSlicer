#include "Slic3r/Biz/ResultExport/ExportNameParser.hpp"

#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Domain/PrintStatistics.hpp"
#include "Slic3r/Biz/Units.hpp"

#include "libslic3r/SLA/SLAResult.hpp"

#include <boost/filesystem/path.hpp>
#include <algorithm>

namespace Slic3r::Biz::ExportNameParser {

namespace {

std::string resolve_preffered_extension(const std::string& output_filename_format)
{
    return boost::filesystem::path(output_filename_format).extension().string();
}

std::string remove_spaces(std::string s) {
    s.erase(std::remove_if(s.begin(), s.end(), ::isspace), s.end());
    return s;
}

ExportNameData parse_fdm_export_name(
    const std::string& project_name,
    const std::string& output_filename_format,
    Domain::ConfigPackFDM* fdm_config,
    const Biz::ProjectInteractor* project_interactor 
)
{
    ExportNameData result {Technology::Fdm, project_name, resolve_preffered_extension(output_filename_format)};
    Domain::FullConfigFDM full_config{*fdm_config};
    Biz::Parser::IO::Config io_config = Biz::Parser::IO::get_parser_config(full_config);
    Biz::Parser::PlaceholderParser parser{io_config};
    const std::optional<Biz::FDMResultRef> fdm_result_opt{project_interactor->fdm_result_cache().get_result(project_interactor->selected_bed_slicing_id())};
    if (!fdm_result_opt) {
         SPDLOG_ERROR("Failed to parse output filename: Failed to retrieve FDM slicing result.");
        return result;
    }
    const Biz::FDMResultRef fdm_result = fdm_result_opt.value();

    parser.set("input_filename_base", project_name);
    const Domain::PrintStatistics& print_statistics = fdm_result.get().print_statistics;
    if (!print_statistics.extra) {
        // We could also fill just basic. The filename will look bad though. 
        // Extra stats are present only after slicing.
        SPDLOG_ERROR("Failed to parse output filename: FDM Statistics are incomplete.");
        return result;
    }

    // basic statistics
    double total_filament_length{0.};
    double total_filament_weight{0.};
    for (const auto& [_, len] : print_statistics.basic.used_filaments_per_role) {
        total_filament_length += (double) len.first;
        total_filament_weight += (double) len.second;
    }

    double total_volume{0.};
    for (const auto& [id, volume] : print_statistics.basic.volumes_per_extruder) {
        total_volume += (double) volume;
    }

    double total_cost{0.};
    for (const auto& [id, cost] : print_statistics.basic.cost_per_extruder) {
        total_cost += (double) cost;
    }
    
    std::string printing_filament_types = print_statistics.extra.value().printing_filament_types.front();
    for (size_t i = 1; i < print_statistics.extra.value().printing_filament_types.size(); ++ i) {
        printing_filament_types += ",";
        printing_filament_types += print_statistics.extra.value().printing_filament_types[i];
    }

    if (print_statistics.basic.silent_mode_time) {
        parser.set("silent_print_time", remove_spaces(Biz::format_time_dhms_short(print_statistics.basic.silent_mode_time.value().time)));
    } else {
        parser.set("silent_print_time", std::string());
    }
    parser.set("print_time", remove_spaces(Biz::format_time_dhms_short(print_statistics.basic.normal_mode_time.time)));
    parser.set("normal_print_time", remove_spaces(Biz::format_time_dhms_short(print_statistics.basic.normal_mode_time.time)));
    parser.set("used_filament", total_filament_length);
    parser.set("total_weight", total_filament_weight);
    parser.set("extruded_volume", total_volume);
    parser.set("total_cost", total_cost);
    parser.set("num_extruders", (int)print_statistics.basic.volumes_per_extruder.size());

    // extra statistics
    parser.set("printing_filament_types", printing_filament_types);
    parser.set("initial_tool", (int) print_statistics.extra.value().initial_extruder_id);
    parser.set("initial_extruder", (int) print_statistics.extra.value().initial_extruder_id);
    parser.set("num_printing_extruders", (int) print_statistics.extra.value().printing_extruders.size());
    parser.set("total_wipe_tower_cost", print_statistics.extra.value().total_wipe_tower_cost);
    parser.set("total_wipe_tower_filament", print_statistics.extra.value().total_wipe_tower_filament);
    parser.set("initial_filament_type", print_statistics.extra.value().initial_filament_type);
    parser.set("total_toolchanges", (int) print_statistics.extra.value().total_toolchanges);
    
    // values from project interactor
    Domain::SelectionId project_id = project_interactor->selected_bed_slicing_id().project_id;
    const auto& project = project_interactor->workbench().project(project_id);
    const auto& ccc     = project_interactor->preset_interactor().selected_config_container_context();
    const auto* cc      = project.find_config_container(ccc.config_container_id);
    ASSERT(cc != nullptr);
    const auto& selected_preset = cc->selected_preset();
    const auto& hw_config_id    = selected_preset.hw_config.id;
    const auto& printer_id      = selected_preset.printer.id;
    const auto& materials      = selected_preset.materials;
    const auto& print_id = selected_preset.print.id;

    std::string printer_name = remove_spaces(project_interactor->preset_interactor().get_printer_preset(hw_config_id, printer_id).first.get().name);
    std::string print_name = remove_spaces(project_interactor->preset_interactor().get_print_preset(hw_config_id, printer_id, print_id).first.get().name);
    std::string filament_name;
    for (size_t i = 0; i < materials.size(); i++) {
        const auto& material_id      = materials[i].id;
        filament_name += remove_spaces(project_interactor->preset_interactor().get_material_preset(hw_config_id, printer_id, print_id, i, material_id).first.get().name);
        if (i != materials.size() - 1) {
            filament_name+=",";
        }
    }

    const auto* bed_instance = project.find_bed_instance_by_id(project_interactor->selected_bed_slicing_id().bed_instance_id);
    size_t model_instances_size = bed_instance->model_instances.size();

    std::vector<std::string> names;
    names.reserve(bed_instance->model_instances.size());
    for (const auto* inst : bed_instance->model_instances) {
        if (inst && inst->get_object()) {
            names.push_back(inst->get_object()->name);
        }
    }
    std::ranges::sort(names);
    const auto [first, last] = std::ranges::unique(names);
    names.erase(first, last);

    parser.set("printer_preset", printer_name);
    parser.set("print_preset", print_name);
    parser.set("filament_preset", filament_name);
    parser.set("num_objects", (int)names.size());
    parser.set("num_instances", (int)bed_instance->model_instances.size());

    // TODO
    //physical_printer_preset

    // This line might throw PlaceholderParserError
    result.filename = parser.process(output_filename_format);
    return result;
}
}
ExportNameData parse_sla_export_name(
    const std::string& project_name,
    const std::string& output_filename_format,
    Domain::ConfigPackSLA* sla_config,
    const Biz::ProjectInteractor* project_interactor
)
{
    ExportNameData result {Technology::Sla, project_name, resolve_preffered_extension(output_filename_format)};
    Domain::FullConfigSLA full_config{*sla_config};
    Biz::Parser::IO::Config io_config = Biz::Parser::IO::get_parser_config(full_config);
    Biz::Parser::PlaceholderParser parser{io_config};
    const Biz::SLAResultOptRef sla_result_opt{project_interactor->sla_result_cache().get_result(project_interactor->selected_bed_slicing_id())};
    if (!sla_result_opt) {
        SPDLOG_ERROR("Failed to parse output filename: Failed to retrieve SLA slicing result.");
        return result;
    }
    const Biz::SLAResultRef sla_result = sla_result_opt.value();

    parser.set("input_filename_base", project_name);
    const std::optional<Biz::Slicing::Sla::PrintStatistics> print_statistics = sla_result.get().print_statistics;
    if (!print_statistics) {
        SPDLOG_ERROR("Failed to parse output filename: Failed to retrieve SLA print statistics.");
        return result;
    }
    parser.set("print_time", print_statistics.value().estimated_print_time);
    parser.set("total_cost", print_statistics.value().total_cost);
    parser.set("total_weight", print_statistics.value().total_weight);
    parser.set("objects_used_material", print_statistics.value().objects_used_material);
    parser.set("support_used_material", print_statistics.value().support_used_material);
    
    Domain::SelectionId project_id = project_interactor->selected_bed_slicing_id().project_id;
    const auto& project = project_interactor->workbench().project(project_id);
    const auto* bed_instance = project.find_bed_instance_by_id(project_interactor->selected_bed_slicing_id().bed_instance_id);

    std::vector<std::string> names;
    names.reserve(bed_instance->model_instances.size());
    for (const auto* inst : bed_instance->model_instances) {
        if (inst && inst->get_object()) {
            names.push_back(inst->get_object()->name);
        }
    }
    std::ranges::sort(names);
    const auto [first, last] = std::ranges::unique(names);
    names.erase(first, last);

    parser.set("num_objects", (int)names.size());
    parser.set("num_instances", (int)bed_instance->model_instances.size());

    // This line might throw PlaceholderParserError
    result.filename = parser.process(output_filename_format);
    return result;
}

ExportNameData parse_export_name(const Biz::ProjectInteractor* project_interactor)
{
    Domain::SelectionId project_id = project_interactor->selected_project_id();
    std::string project_name = project_interactor->get_project_name(project_id);
    const Domain::ConfigItem*
        item = project_interactor->preset_interactor().selected_printer_preset().print.config_box().items.find("output_filename_format");
    if (!item) {
        SPDLOG_ERROR("Failed to parse output filename: Failed to retrieve Config Item.");
        return {Technology::Fdm, project_name, {}};
    }
    // Create placeholder parser.
    // Feed it with selected presets and print statistics.
    std::string output_filename_format = item->get<std::string>();
    Domain::ConfigPack config = project_interactor->preset_interactor().selected_printer_preset().config();

    if (auto* fdm_config = std::get_if<Domain::ConfigPackFDM>(&config)) {        
        return ExportNameParser::parse_fdm_export_name(project_name, output_filename_format, fdm_config, project_interactor);
    } else if (auto* sla_config = std::get_if<Domain::ConfigPackSLA>(&config)) {
        return ExportNameParser::parse_sla_export_name(project_name, output_filename_format, sla_config, project_interactor);
    } else {
        SPDLOG_ERROR("Failed to parse output filename: Failed to retrieve a slicing result.");
    }
    return {Technology::Fdm, project_name, resolve_preffered_extension(output_filename_format)};
}

ExportNameData error_state_export_name(const Biz::ProjectInteractor* project_interactor)
{
       Domain::SelectionId project_id = project_interactor->selected_project_id();
    std::string project_name = project_interactor->get_project_name(project_id);
    const Domain::ConfigItem*
        item = project_interactor->preset_interactor().selected_printer_preset().print.config_box().items.find("output_filename_format");
    if (!item) {
        SPDLOG_ERROR("Failed to parse output filename: Failed to retrieve Config Item.");
        return {Technology::Fdm, project_name, {}};
    }
    std::string output_filename_format = item->get<std::string>();
    Domain::ConfigPack config = project_interactor->preset_interactor().selected_printer_preset().config();

    if (std::get_if<Domain::ConfigPackFDM>(&config)) {        
        return {Technology::Fdm, project_name, resolve_preffered_extension(output_filename_format)};
    } else if (std::get_if<Domain::ConfigPackSLA>(&config)) {
        return {Technology::Sla, project_name, resolve_preffered_extension(output_filename_format)};
    } else {
        SPDLOG_ERROR("Failed to figure output filename: Failed to retrieve a slicing result.");
    }
    return {Technology::Fdm, project_name, resolve_preffered_extension(output_filename_format)};
}

}