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

class EditorPrinter : public AbstractEditor
{
public:
    EditorPrinter(wxWindow* parent, Biz::Preset::PresetInteractor& preset_interactor);
    ~EditorPrinter() {}

    bool    apply_extruder_cnt_from_cache();
    bool    is_init_extruders_cnt_dirty() { return m_initial_extruders_count != m_extruders_count; }
    bool    is_sys_extruders_cnt_dirty()  { return m_sys_extruders_count     != m_extruders_count; }

    void    set_init_extruders_cnt(size_t init_extruders_cnt)   { m_initial_extruders_count = init_extruders_cnt; }
    void    set_sys_extruders_cnt (size_t sys_extruders_cnt)    { m_sys_extruders_count = sys_extruders_cnt; }

private:
    void    build() override;
    void    build_print_host_upload_group(Page* page);
    void    build_fff();
    void    build_sla();
    void    reload_config() override;
    void    activate_selected_page(std::function<void()> throw_if_canceled) override;
    void    clear_pages() override;
    void    toggle_options() override;
    void    update() override;
    void    update_fff();
    void    update_sla();
    void    update_pages(); // update m_pages according to printer technology
    void    extruders_count_changed(size_t extruders_count);
    PageShp build_kinematics_page();
    void    build_extruder_pages(size_t n_before_extruders);
    void    build_unregular_pages(bool from_initial_build = false);
    void    append_option_line(ConfigOptionsGroupShp optgroup, const std::string opt_key);
    void    update_machine_limits_description(const MachineLimitsUsage usage);

    void    on_preset_loaded() override;
    void    init_options_list() override;

    wxSizer*    create_bed_shape_widget(wxWindow* parent);
    void    cache_extruder_cnt(const DynamicPrintConfig* config = nullptr);
    void    update_sla_prusa_specific_visibility() override;

public:
    PrinterTechnology   printer_technology{ ptFFF };

private:
    size_t  m_extruders_count           { 0 };
    size_t  m_extruders_count_old       { 0 };
    size_t  m_initial_extruders_count   { 0 };
    size_t  m_sys_extruders_count       { 0 };
    size_t  m_cache_extruder_count      { 0 };


    bool    m_has_single_extruder_MM_page   { false };
    bool    m_use_silent_mode               { false };
    bool    m_supports_travel_acceleration  { false };
    bool    m_supports_min_feedrates        { false };
    bool    m_rebuild_kinematics_page       { false };

    ogStaticText*   m_machine_limits_description_line           {nullptr};
    ogStaticText*   m_fff_print_host_upload_description_line    {nullptr};
    ogStaticText*   m_sla_print_host_upload_description_line    {nullptr};

    std::vector<PageShp>    m_pages_fff;
    std::vector<PageShp>    m_pages_sla;
};

} 

