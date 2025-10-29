#pragma once

#include "PresetDiffOperation.hpp"
#include "Slic3r/Domain/ConfigPack.hpp"

namespace Slic3r::Biz::Preset {

struct PresetSelectionNames;

class IPresetDialogManager
{
public:
    IPresetDialogManager() = default;
    virtual ~IPresetDialogManager() = default;

    virtual PresetDiffOperation show_unsaved_changes_dialog(
        const Domain::ConfigPack& config_original,
        const Domain::ConfigPack& config_selected,
        Domain::ConfigPack* config_new_selected,
        const PresetSelectionNames& preset_names,
        const PresetSelectionNames& preset_names_new
    ) = 0;
};
} // namespace Slic3r::Biz::Preset
