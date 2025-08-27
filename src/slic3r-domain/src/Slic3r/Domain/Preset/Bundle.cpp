#include "Slic3r/Domain/Preset/Bundle.hpp"

namespace Slic3r::Domain::Preset {

const HwPrinterConfig* VendorBundle::find_printer_config(const std::string& id) const
{
    auto it = std::ranges::
        find_if(printer_configs, [&id](const HwPrinterConfig& config) { return config.id == id; });
    if (it == printer_configs.end())
        return nullptr;
    return &(*it);
}

HwPrinterConfig* VendorBundle::find_printer_config(const std::string& id)
{
    auto it = std::ranges::
        find_if(printer_configs, [&id](const HwPrinterConfig& config) { return config.id == id; });
    if (it == printer_configs.end())
        return nullptr;
    return &(*it);
}


const EvaluatedPrinterPreset* Bundle::find_printer_preset(
    const std::string& printer_hw_config_id,
    const std::string& printer_preset_id
) const
{
    const auto& printer_presets = evaluated_presets.find(printer_hw_config_id);

    if (printer_presets == evaluated_presets.end())
        return nullptr;

    auto it = std::ranges::find_if(printer_presets->second, [&printer_preset_id](const EvaluatedPrinterPreset& p) {
        return p.preset.id == printer_preset_id;
    });
    if (it != printer_presets->second.end())
        return &*it;
    return nullptr;
}

const HwPrinterConfig* Bundle::find_config_with_same_values(const HwPrinterConfig& printer_config) const
{
    auto it = std::ranges::find_if(
        printer_configs,
        [&printer_config](const auto& pair) { return pair.second.has_same_values(printer_config); }
    );
    return it == printer_configs.end() ? nullptr : &(it->second);
}

const EvaluatedPrinterPreset* Bundle::find_printer_preset_with_same_values(
    const std::string& hw_config_id,
    const EvaluatedPrinterPreset::Preset& printer_preset
) const
{
    const auto& evaluated_printer_presets = evaluated_presets.at(hw_config_id);
    return find_preset_with_same_value(printer_preset, evaluated_printer_presets);
}

} // namespace Slic3r::Domain::Preset
