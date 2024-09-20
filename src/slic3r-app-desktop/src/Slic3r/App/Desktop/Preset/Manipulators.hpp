///|/ Copyright (c) Prusa Research 2017 - 2023 Oleksandra Iushchenko @YuSanka, Tomáš Mészáros @tamasmeszaros, Vojtěch Bubník @bubnikv, Lukáš Matěna @lukasmatena, Enrico Turri @enricoturri1966, Vojtěch Král @vojtechkral
///|/ Copyright (c) 2019 John Drake @foxox
///|/ Copyright (c) 2018 Martin Loidl @LoidlM
///|/
///|/ ported from lib/Slic3r/GUI/Tab.pm:
///|/ Copyright (c) Prusa Research 2016 - 2018 Vojtěch Bubník @bubnikv, Lukáš Matěna @lukasmatena
///|/ Copyright (c) 2015 - 2017 Joseph Lenox @lordofhyphens
///|/ Copyright (c) Slic3r 2012 - 2016 Alessandro Ranellucci @alranel
///|/ Copyright (c) 2016 Chow Loong Jin @hyperair
///|/ Copyright (c) 2012 QuantumConcepts
///|/ Copyright (c) 2012 Henrik Brix Andersen @henrikbrixandersen
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "libslic3r/Preset.hpp"
#include <wx/sizer.h>

#include <functional>
#include <vector>

class wxWindow;

namespace Slic3r::App::WX {
    class ScalableButton;
}

namespace Slic3r::Biz::Preset {
    class   PresetInteractor;
    struct  PresetState;
}

namespace Slic3r::App::Desktop::Preset {

class EditorPresetComboBox;

class Manipulators : wxBoxSizer
{
public:
    Manipulators(wxWindow*                      parent, 
                 Biz::Preset::PresetInteractor* preset_interactor,
                 Slic3r::Preset::Type           type);
    virtual ~Manipulators() {}

    void    update(const Biz::Preset::PresetState* state, const std::string& printer_model, const std::string& ph_printer_name);
    void    show_btn_incompatible_presets(bool show = true);
    void    sys_color_changed();

private:

    WX::ScalableButton* add_button(const std::string&    icon_name,
                                   const wxString&       tooltip,
                                   std::function<void()> fn_on_click = nullptr,
                                   std::function<bool()> fn_ui_update = nullptr,
                                   int                   left_space = 10);

    void    edit_physical_printer();
    void    add_physical_printer();
    bool    del_physical_printer(const wxString& note_string = wxEmptyString);

    void    transfer_options(const std::string& name_from, const std::string& name_to, std::vector<std::string> options);
    void    save_preset(std::string name = std::string(), bool detach = false);
    void    rename_preset();
    void    detach_preset();
    void    delete_preset();
    void    toggle_show_hide_incompatible();
    void    update_compatibility_ui();
    void    compare_preset();

 
private:
    wxWindow*                           m_parent            { nullptr };
    Biz::Preset::PresetInteractor*      m_preset_interactor { nullptr };
    const Biz::Preset::PresetState*     m_preset_state      { nullptr };
    Slic3r::Preset::Type                m_type;
    std::string                         m_printer_model     {};
    // Name of the selected physical printer. Can has value just for TYPE_PRINTER 
    std::string                         m_ph_printer_name   {};

    std::vector<WX::ScalableButton*>    m_buttons = {};

    WX::ScalableButton* m_btn_compare_preset                { nullptr };
    WX::ScalableButton* m_btn_save_preset                   { nullptr };
    WX::ScalableButton* m_btn_rename_preset                 { nullptr };
    WX::ScalableButton* m_btn_delete_preset                 { nullptr };
    WX::ScalableButton* m_btn_edit_ph_printer               { nullptr };
    WX::ScalableButton* m_btn_hide_incompatible_presets     { nullptr };
    WX::ScalableButton* m_detach_preset_btn                 { nullptr };

    bool                m_can_rename_presets                { false };
    bool                m_can_detach_presets                { false };
    bool                m_can_delete_presets                { false };
    bool				m_show_btn_incompatible_presets     { false };
    bool                m_show_incompatible_presets         { false };
};

} 

