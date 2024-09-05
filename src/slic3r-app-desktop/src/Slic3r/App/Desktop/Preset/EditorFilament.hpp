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

#include "AbstractEditor.hpp"

class wxWindow;

namespace Slic3r::App::Desktop::Preset {

class EditorFilament : public AbstractEditor
{
public:
    EditorFilament(wxWindow* parent, Biz::Preset::PresetInteractor& preset_interactor);
    ~EditorFilament() = default;

private:
    void    build() override;
    void    update_description_lines() override;
    void    toggle_options() override;
    void    update() override;
    void    clear_pages() override;
    void    msw_rescale() override;
    void    sys_color_changed() override;
    bool    supports_printer_technology(const PrinterTechnology tech) const override { return tech == ptFFF; }

    const std::string&  get_custom_gcode(const t_config_option_key& opt_key) override;
    void                set_custom_gcode(const t_config_option_key& opt_key, const std::string& value) override;

    void    create_line_with_near_label_widget(ConfigOptionsGroupShp optgroup, const std::string &opt_key, int opt_index = 0);
    void    update_line_with_near_label_widget(ConfigOptionsGroupShp optgroup, const std::string &opt_key, int opt_index = 0, bool is_checked = true);
    void    add_filament_overrides_page();
    void    update_filament_overrides_page();
    void    update_volumetric_flow_preset_hints();

private:
    int             m_active_extruder                   { 0 };
    ogStaticText*   m_volumetric_speed_description_line { nullptr };
    ogStaticText*   m_cooling_description_line          { nullptr };

    std::map<std::string, wxWindow*> m_overrides_options;
};

} 

