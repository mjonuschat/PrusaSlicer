///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/AppSettingsAdvanced.hpp"

#include <nlohmann/json.hpp>

namespace Slic3r::App {

void AppSettingsAdvanced::toggle_printer_favorite_preset(
    const std::string& id,
    const std::string& hw_config_id
)
{
    // const std::pair<std::string, std::string> preset_id{id, hw_config_id};
    if (printer_favorite_presets.contains(id)) {
        printer_favorite_presets.erase(id);
    } else {
        printer_favorite_presets.insert(id);
    }
}

void AppSettingsAdvanced::toggle_material_favorite_preset(const std::string& id)
{
    if (material_favorite_presets.contains(id)) {
        material_favorite_presets.erase(id);
    } else {
        material_favorite_presets.insert(id);
    }
}

bool AppSettingsAdvanced::contains_printer_favorite_preset(
    const std::string& id,
    const std::string& hw_config_id
) const
{
    return printer_favorite_presets.contains(id);
}

bool AppSettingsAdvanced::contains_material_favorite_preset(const std::string& id) const
{
    return material_favorite_presets.contains(id);
}

void to_json(
    nlohmann::ordered_json& json_value,
    const Slic3r::App::AppSettingsAdvanced& app_settings_advanced
)
{
    json_value = {
        {"printer_favorite_presets", app_settings_advanced.printer_favorite_presets},
        {"material_favorite_presets", app_settings_advanced.material_favorite_presets}
    };
}

void from_json(
    const nlohmann::ordered_json& json_value,
    Slic3r::App::AppSettingsAdvanced& app_settings_advanced
)
{
    json_value.at("printer_favorite_presets")
        .get_to(app_settings_advanced.printer_favorite_presets);
    json_value.at("material_favorite_presets")
        .get_to(app_settings_advanced.material_favorite_presets);
}

} // namespace Slic3r::App
