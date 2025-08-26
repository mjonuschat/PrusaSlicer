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
    const auto& printer_presets = evaluated_presets.at(printer_hw_config_id);

    auto it = std::ranges::find_if(printer_presets, [&printer_preset_id](const EvaluatedPrinterPreset& p) {
        return p.preset.id == printer_preset_id;
    });
    if (it != printer_presets.end())
        return &*it;
    return nullptr;
}

} // namespace Slic3r::Domain::Preset
