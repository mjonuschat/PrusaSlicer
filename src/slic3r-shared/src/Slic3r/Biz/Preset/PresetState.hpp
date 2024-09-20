#pragma once

#include "libslic3r/Preset.hpp"

namespace Slic3r::Biz::Preset {

struct PresetState
{
    Slic3r::Preset*         selected_preset{ nullptr };
    const Slic3r::Preset*   selected_preset_parent{ nullptr };
    Slic3r::Preset          edited_preset;

    PresetState() = default;
    PresetState(const PresetState&) = default;
    PresetState& operator=(const PresetState&) = default;

    PresetState(Slic3r::Preset* selected_preset_in, const Slic3r::Preset* selected_preset_parent_in) :
        selected_preset(selected_preset_in),
        selected_preset_parent(selected_preset_parent_in) {
        edited_preset = *selected_preset;
    }

    // Compare the content of get_selected_preset() with get_edited_preset() configs, return true if they differ.
    bool                        current_is_dirty() const
    {
        return PresetCollection::is_dirty(&edited_preset, selected_preset);
    }
    // Compare the content of get_selected_preset() with get_edited_preset() configs, return the list of keys where they differ.
    std::vector<std::string>    current_dirty_options(const bool deep_compare = false) const
    {
        return PresetCollection::dirty_options(&edited_preset, selected_preset, deep_compare);
    }
    // Compare the content of get_selected_preset() with get_edited_preset() configs, return the list of keys where they differ.
    std::vector<std::string>    current_different_from_parent_options(const bool deep_compare = false) const
    {
        return PresetCollection::dirty_options(&edited_preset, selected_preset_parent, deep_compare);
    }

};

}
