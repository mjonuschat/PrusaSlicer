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

class EditorSLAMaterial : public AbstractEditor
{
public:
    EditorSLAMaterial(wxWindow* parent, Biz::Preset::PresetInteractor& preset_interactor);
    ~EditorSLAMaterial() {}

private:
    void    build() override;
    void    build_tilt_group(PageShp page);
    void    toggle_tilt_options(bool is_above);
    void    toggle_options() override;
    void    update() override;
    void    clear_pages() override;
    void    msw_rescale() override;
    void    sys_color_changed() override;
    void    update_sla_prusa_specific_visibility() override;
    void    update_description_lines() override;
    bool    supports_printer_technology(const PrinterTechnology tech) const override { return tech == ptSLA; }

    void    create_line_with_near_label_widget(ConfigOptionsGroupShp optgroup, const std::string& opt_key);
    void    update_line_with_near_label_widget(ConfigOptionsGroupShp optgroup, const std::string& opt_key, bool is_checked = true);
    void    add_material_overrides_page();
    void    update_material_overrides_page();

private:
    std::map<std::string, wxWindow*>    m_overrides_options;
    ogStaticText*                       m_z_correction_to_mm_description{ nullptr };

};

} 

