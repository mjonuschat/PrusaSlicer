///|/ Copyright (c) Prusa Research 2020 - 2023 Oleksandra Iushchenko @YuSanka, David Kocík @kocikdav, Lukáš Matěna @lukasmatena
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

// This dialog is used for rename or save preset with new name.
// Containes one or several PresetNameGetter(s)

#include <wx/dialog.h>

#include "libslic3r/Preset.hpp"

class wxString;
class wxStaticText;
class wxCheckBox;
class wxTextCtrl;
class wxStaticBitmap;

namespace Slic3r::App::Desktop::Preset {

class PresetNameGetter;

class SavePresetDialog : public wxDialog
{
public:
    SavePresetDialog(wxWindow* parent, std::vector<const Slic3r::Preset*> selected_presets, const PresetBundle* preset_bundle, std::string suffix = "", bool template_filament = false, std::string ph_printer_name = "");
    SavePresetDialog(wxWindow* parent, const Slic3r::Preset* selected_preset, const PresetBundle* preset_bundle);
    ~SavePresetDialog() override;

    void    AddItem(const Slic3r::Preset* selected_preset, const std::string& suffix, bool is_for_multiple_save);
    bool    Layout() override;

    void    set_info_line_extentions(const wxString& info_line_extention);
    bool    get_template_filament_checkbox();

    std::string     get_name();
    std::string     get_name(Slic3r::Preset::Type type);

private:
    void    build(std::vector<const Slic3r::Preset*> selected_presets, std::string suffix = "", bool template_filament = false);
    void    add_info_for_edit_ph_printer(wxBoxSizer *sizer);
    void    update_info_for_edit_ph_printer(bool is_valid_name, const std::string &preset_name);
    void    update_physical_printers(const std::string& preset_name);
    void    accept();
    bool    enable_ok_btn() const;
    void    dpi_changed();

private:
    enum ActionType
    {
        ChangePreset,
        AddPreset,
        Switch,
        UndefAction
    };

    std::vector<PresetNameGetter*>                  m_items;

    const PresetBundle*   m_preset_bundle           { nullptr };
    wxBoxSizer*     m_presets_sizer                 { nullptr };
    wxStaticText*   m_label                         { nullptr };
    wxBoxSizer*     m_radio_sizer                   { nullptr };  
    ActionType      m_action                        { UndefAction };
    wxCheckBox*     m_template_filament_checkbox    { nullptr };

    bool            m_use_for_rename                { false };
    wxString        m_info_line_extention           { wxEmptyString };
    std::string     m_ph_printer_name               { std::string() };
    std::string     m_old_preset_name               { std::string() };
};

} 
