#pragma once

#include "PresetDiffOperation.hpp"
#include "Slic3r/Domain/ConfigPack.hpp"
#include "Slic3r/Domain/Preset/Types.hpp"

namespace Slic3r::Biz::Preset {

struct PresetSelectionNames;
class PresetInteractor;

struct PresetSwitchKindId
{
    Domain::Preset::PresetKind kind;
    std::optional<size_t> id{std::nullopt};

    bool operator<(const PresetSwitchKindId& other) const
    {
        if (kind == other.kind) {
            if (id && other.id) {
                return id.value() < other.id.value();
            }
            return false; //?
        }
        return uint8_t(kind) < uint8_t(other.kind);
    }

    bool operator==(const PresetSwitchKindId& other) const
    {
        const bool equal_ids = (id && other.id && id.value() == other.id.value()) || (!id && !other.id);
        return kind == other.kind && equal_ids;
    }
};

class IPresetDialogManager
{
public:
    IPresetDialogManager()          = default;
    virtual ~IPresetDialogManager() = default;

    using PresetsSwitchStates = std::map<PresetSwitchKindId, PresetSwitchState>;

    virtual PresetsSwitchStates show_unsaved_changes_dialog(
        const std::string& dialog_name,
        const Domain::ConfigPack& config_original,
        const Domain::ConfigPack& config_selected,
        Domain::ConfigPack* config_new_selected,
        const PresetSelectionNames& preset_names,
        const PresetSelectionNames& preset_names_new,
        const PresetInteractor& preset_interactor
    ) = 0;
};
} // namespace Slic3r::Biz::Preset
