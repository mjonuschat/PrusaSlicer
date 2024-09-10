///|/ Copyright (c) Prusa Research 2020 - 2023 Oleksandra Iushchenko @YuSanka, David Kocík @kocikdav, Lukáš Matěna @lukasmatena
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

// Class to get new preset name
// Contains TextCtrl or editable ComboBox, info icon and info line(s) about entered name
// Is used as an item for SavePresetDialog or separate item to check entered preset name (f.e in ConfigWizard)

#include "libslic3r/Preset.hpp"

#include <wx/string.h>
#include <functional>

class wxWindow;
class wxBoxSizer;
class wxStaticText;
class wxCheckBox;
class wxTextCtrl;
class wxStaticBitmap;

namespace Slic3r::App::WX::Widgets{
    class ComboBox;
}

namespace Slic3r::App::Desktop::Preset {

class PresetNameGetter
{
public:
    enum class ValidationType
    {
        Valid,
        NoValid,
        Warning
    };

    // To create a PresetNameGetter as an item inside of the SavePresetDialog
    PresetNameGetter(wxWindow* parent, wxBoxSizer* sizer, const Slic3r::Preset* selected_preset, PresetCollection* presets, const std::string& suffix, bool as_text_ctrl, bool show_label);

    // To create a PresetNameGetter as a separate control(f.e. as a part of ConfigWizard to check name of the new custom priter)
    PresetNameGetter(wxWindow* parent, wxBoxSizer* sizer, const std::string& def_name, PresetBundle* preset_bundle, PrinterTechnology pt = ptFFF);

    bool                    is_valid()      const { return m_valid_type != ValidationType::NoValid; }
    Slic3r::Preset::Type    type()          const { return m_type; }
    std::string             preset_name()   const;

    void    enable(bool enable = true);
    void    update();
    void    update_valid_bmp();

    void    set_cb_info_line_extention(std::function<wxString()> cb)                        { m_cb_info_line_extention    = cb; };
    void    set_cb_update_extra_info_line(std::function<void(bool, const std::string&)> cb) { m_cb_update_extra_info_line = cb; };

private:
    void    build(wxBoxSizer* sizer, std::string preset_name, bool show_label = false);
    void    init_input_name_ctrl(wxBoxSizer *input_name_sizer, std::string preset_name);
    void    init_casei_preset_names();

    std::string             get_init_preset_name(const Slic3r::Preset* selected_preset, const std::string &suffix);
    std::string             get_conflict_name(const std::string& preset_name) const;
    const Slic3r::Preset*   get_existing_preset() const;

private:
    struct PresetName {
        std::string casei_name;
        std::string name;

        bool operator<(const PresetName& other) const { return other.casei_name > this->casei_name; }
    };

    Slic3r::Preset::Type    m_type                  { Slic3r::Preset::TYPE_INVALID };
    bool                    m_use_text_ctrl         { true };

    PrinterTechnology       m_printer_technology    { ptAny };
    ValidationType          m_valid_type            { ValidationType::NoValid };
    wxWindow*               m_parent                { nullptr };
    wxStaticBitmap*         m_valid_bmp             { nullptr };
    WX::Widgets::ComboBox*  m_combo                 { nullptr };
    wxTextCtrl*             m_text_ctrl             { nullptr };
    wxStaticText*           m_valid_label           { nullptr };
    PresetCollection*       m_presets               { nullptr };
    PresetBundle*           m_preset_bundle         { nullptr };

    std::string		        m_preset_name;
    std::vector<PresetName> m_casei_preset_names;

    // Callbacks from parent to extend informatin lines, if needed
    std::function<wxString()>                       m_cb_info_line_extention        { nullptr };
    std::function<void(bool, const std::string&)>   m_cb_update_extra_info_line     { nullptr };
};

} 
