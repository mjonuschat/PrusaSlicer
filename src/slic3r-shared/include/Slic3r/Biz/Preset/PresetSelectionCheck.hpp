#pragma once

#include <string>

namespace Slic3r::Biz::Preset {

class PresetInteractor;

namespace PresetSelectionCheck {

bool can_select_printer_preset(
    PresetInteractor& preset_interactor,
    const std::string& printer_hw_config_id,
    const std::string& printer_preset_id
);

bool can_select_print_preset(
    PresetInteractor& preset_interactor, 
    const std::string& id
);

bool can_select_tool_print_preset(
    PresetInteractor& preset_interactor,
    size_t tool_index,
    const std::string& id
);

bool can_select_material_preset(
    PresetInteractor& preset_interactor,
    size_t material_index,
    const std::string& id
);

} // namespace PresetSelectionCheck

} // namespace Slic3r::Biz::Preset
