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

class EditorSLAPrint : public AbstractEditor
{
public:
    EditorSLAPrint(wxWindow* parent, Biz::Preset::PresetInteractor& preset_interactor);
    ~EditorSLAPrint() {}

private:
    void    build() override;
    void    update_description_lines() override;
    void    toggle_options() override;
    void    update() override;
    void    clear_pages() override;
    bool    supports_printer_technology(const PrinterTechnology tech) const override { return tech == ptSLA; }

    // Methods are a vector of method prefix -> method label pairs
    // method prefix is the prefix whith which all the config values are prefixed
    // for a particular method. The label is the friendly name for the method
    void build_sla_support_params(const std::vector<SamePair<std::string>> &methods, const PageShp &page);

private:
    ogStaticText* m_support_object_elevation_description_line = nullptr;
};

} 

