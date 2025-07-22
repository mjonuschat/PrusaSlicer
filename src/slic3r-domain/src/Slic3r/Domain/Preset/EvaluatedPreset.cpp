#include "Slic3r/Domain/Preset/EvaluatedPreset.hpp"

namespace Slic3r::Domain::Preset {

const EvaluatedToolPrintPreset* EvaluatedPrintPreset::find_tool_preset_by_id(
    size_t tool_idx,
    const std::string& id
) const
{
    const auto& tool_presets = tools[tool_idx];
    auto it = std::ranges::find_if(tool_presets, [&id](const EvaluatedToolPrintPreset& p) {
        return p.preset.id == id;
    });
    return it == tool_presets.end() ? nullptr : &*it;
}

const EvaluatedPrintPreset* EvaluatedPrinterPreset::find_print_preset_by_id(const std::string& id) const
{
    auto it = std::ranges::find_if(prints, [&id](const EvaluatedPrintPreset& p) {
        return p.preset.id == id;
    });
    return it == prints.end() ? nullptr : &*it;
}

const EvaluatedMaterialPreset* EvaluatedPrinterPreset::find_material_preset_by_id(
    size_t tool_idx,
    const std::string& id
) const
{
    const auto& tool_materials = materials[tool_idx];
    auto it = std::ranges::find_if(tool_materials, [&id](const EvaluatedMaterialPreset& p) {
        return p.preset.id == id;
    });
    return it == tool_materials.end() ? nullptr : &*it;
}

bool EvaluatedPrinterPreset::is_valid() const
{
    return !prints.empty() && std::ranges::all_of(prints, [](const EvaluatedPrintPreset& p) {
        return !p.tools.empty();
    });
}

namespace {
const std::string EMPTY;
} // namespace

std::string SelectedPreset::bed_model() const
{
    auto it = printer.config_box().contains("bed_custom_model");
    if (it.item) {
        std::string filename = it.item->get<std::string>();
        if (!filename.empty())
            return filename;
    }
    return hw_config.visual.bed_model.value_or(std::string());
}

std::string SelectedPreset::bed_texture() const
{
    auto it = printer.config_box().contains("bed_custom_texture");
    if (it.item) {
        std::string filename = it.item->get<std::string>();
        if (!filename.empty())
            return filename;
    }
    return hw_config.visual.bed_texture.value_or(std::string());
}

ConfigPack SelectedPreset::config() const
{
    if (printer.kind == PresetKind::FdmPrinter) {
        ConfigPackFDM config;
        config.printer = std::get<PrinterSettings>(printer.values);
        config.print   = std::get<PrintSettings>(print.values);
        for (const auto& t : tools)
            config.tool.push_back(std::get<ToolPrintSettings>(t.values));
        for (const auto& m : materials)
            config.filament.push_back(std::get<FilamentSettings>(m.values));
        return config;
    }
    if (printer.kind == PresetKind::SlaPrinter) {
        ConfigPackSLA config;
        config.sla_printer_settings = std::get<SLAPrinterSettings>(printer.values);
        config.sla_print_settings   = std::get<SLAPrintSettings>(print.values);
        ASSERT(materials.size() == 1);
        config.sla_material_settings = std::get<SLAMaterialSettings>(materials.at(0).values);
        return config;
    }

    PANIC("Unsupported technology");
}

} // namespace Slic3r::Domain::Preset
