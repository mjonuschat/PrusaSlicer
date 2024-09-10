///|/ Copyright (c) Prusa Research 2020 - 2023 Oleksandra Iushchenko @YuSanka, David Kocík @kocikdav, Vojtěch Bubník @bubnikv, Lukáš Matěna @lukasmatena
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <wx/bmpbndl.h>
#include <wx/gdicmn.h>

#include "PresetComboBox.hpp"
#include "libslic3r/Preset.hpp"

class wxString;
class wxCommandEvent;

namespace Slic3r::Biz::Preset {
    struct  PresetState;
    struct  PresetBundleRuntime;
}

namespace Slic3r::App::Desktop::Preset {

class EditorPresetComboBox : public PresetComboBox
{
public:
    EditorPresetComboBox(wxWindow *parent, Slic3r::Preset::Type preset_type, PresetBundle* preset_bundle);
    ~EditorPresetComboBox() {}

    PresetCollection*       presets()       const   { return m_collection; }
    PresetBundle*           preset_bundle() const   { return m_preset_bundle; }
    Slic3r::Preset::Type    type()          const   { return m_type; }

    void    update(Biz::Preset::PresetState* state, Biz::Preset::PresetBundleRuntime* pb_runtime);
    void    update() override;
    void    update_dirty();
    void    msw_rescale() override;
    void    OnSelect(wxCommandEvent& evt) override;

    void set_show_incompatible_presets(bool show_incompatible_presets) {
        m_show_incompatible = show_incompatible_presets;
    }
private:
    wxString get_preset_name(const Slic3r::Preset& preset) override;

private:
    bool                                m_show_incompatible     { false };

    // Information about selected preset
    Biz::Preset::PresetState*           m_preset_state          { nullptr };
    Biz::Preset::PresetBundleRuntime*   m_pb_runtime            { nullptr };
};

} 
