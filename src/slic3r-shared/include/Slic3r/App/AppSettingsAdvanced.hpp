///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <nlohmann/json_fwd.hpp>

#include <set>
#include <string>

namespace Slic3r::App {

struct AppSettingsAdvanced
{
    using PrinterFavoritePresets = std::set<std::pair<std::string, std::string>>;
    using MaterialFavoritePresets = std::set<std::string>;

    void toggle_printer_favorite_preset(const std::string& id, const std::string& hw_config_id);
    void toggle_material_favorite_preset(const std::string& id);

    PrinterFavoritePresets printer_favorite_presets;
    MaterialFavoritePresets material_favorite_presets;
};

void to_json(
    nlohmann::ordered_json& json_value,
    const Slic3r::App::AppSettingsAdvanced& app_settings_advanced
);
void from_json(
    const nlohmann::ordered_json& json_value,
    Slic3r::App::AppSettingsAdvanced& app_settings_advanced
);

} // namespace Slic3r::App
