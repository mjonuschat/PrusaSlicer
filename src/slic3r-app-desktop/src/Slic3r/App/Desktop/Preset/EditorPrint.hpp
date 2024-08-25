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
#include "SubstitutionManager.hpp"

class wxWindow;
class wxString;

namespace Slic3r::App::Desktop::Preset {

class EditorPrint : public AbstractEditor
{
public:
    EditorPrint(wxWindow* parent);
    ~EditorPrint() {}

private:
    void        build() override;
    void        update_description_lines() override;
    void        toggle_options() override;
    void        update() override;
    void        clear_pages() override;
    bool        supports_printer_technology(const PrinterTechnology tech) const override { return tech == ptFFF; }
    wxSizer*    create_manage_substitution_widget(wxWindow* parent);
    wxSizer*    create_substitutions_widget(wxWindow* parent);

private:
    ogStaticText*       m_recommended_thin_wall_thickness_description_line  {nullptr};
    ogStaticText*       m_top_bottom_shell_thickness_explanation            {nullptr};
    ogStaticText*       m_post_process_explanation                          {nullptr};
    WX::ScalableButton* m_del_all_substitutions_btn                         {nullptr};
    SubstitutionManager m_subst_manager;
};

} 

