///|/ Copyright (c) Prusa Research 2017 - 2023 Oleksandra Iushchenko @YuSanka, Lukáš Matěna @lukasmatena, Lukáš Hejl @hejllukas, Vojtěch Bubník @bubnikv, Pavel Mikuš @Godrak, Tomáš Mészáros @tamasmeszaros, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Vojtěch Král @vojtechkral
///|/ Copyright (c) 2021 Martin Budden
///|/ Copyright (c) 2021 Ilya @xorza
///|/ Copyright (c) 2019 John Drake @foxox
///|/ Copyright (c) 2019 Matthias Urlichs @smurfix
///|/ Copyright (c) 2019 Thomas Moore
///|/ Copyright (c) 2019 Sijmen Schoon
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

#include "EditorSLAPrint.hpp"
#include "../Config/OptionsGroup.hpp"

#include "Slic3r/App/WX/format.hpp"

#include <wx/string.h>

#include <wx/bmpbuttn.h>
#include <wx/wupdlock.h>

//!#include "I18N.hpp"
#define L(s) s
static wxString _L(const wxString& s) { return s; };

namespace Slic3r::App::Desktop::Preset {

void EditorSLAPrint::build_sla_support_params(const std::vector<SamePair<std::string>> &prefixes,
                                           const PageShp &page)
{

    auto optgroup = page->new_optgroup(L("Support head"));
    add_options_into_line(optgroup, prefixes, "support_head_front_diameter");
    add_options_into_line(optgroup, prefixes, "support_head_penetration");
    add_options_into_line(optgroup, prefixes, "support_head_width");

    optgroup = page->new_optgroup(L("Support pillar"));
    add_options_into_line(optgroup, prefixes, "support_pillar_diameter");
    add_options_into_line(optgroup, prefixes, "support_small_pillar_diameter_percent");
    add_options_into_line(optgroup, prefixes, "support_max_bridges_on_pillar");

    add_options_into_line(optgroup, prefixes, "support_pillar_connection_mode");
    add_options_into_line(optgroup, prefixes, "support_buildplate_only");
    add_options_into_line(optgroup, prefixes, "support_pillar_widening_factor");
    add_options_into_line(optgroup, prefixes, "support_max_weight_on_model");
    add_options_into_line(optgroup, prefixes, "support_base_diameter");
    add_options_into_line(optgroup, prefixes, "support_base_height");
    add_options_into_line(optgroup, prefixes, "support_base_safety_distance");

    // Mirrored parameter from Pad page for toggling elevation on the same page
    add_options_into_line(optgroup, prefixes, "support_object_elevation");

    Line line{ "", "" };
    line.full_width = 1;
    line.widget = [this](wxWindow* parent) {
        return description_line_widget(parent, &m_support_object_elevation_description_line);
    };
    optgroup->append_line(line);

    optgroup = page->new_optgroup(L("Connection of the support sticks and junctions"));
    add_options_into_line(optgroup, prefixes, "support_critical_angle");
    add_options_into_line(optgroup, prefixes, "support_max_bridge_length");
    add_options_into_line(optgroup, prefixes, "support_max_pillar_link_distance");
}

EditorSLAPrint::EditorSLAPrint(wxWindow* parent) :
    AbstractEditor(parent, _L("Print Settings"), Slic3r::Preset::TYPE_SLA_PRINT) {}

void EditorSLAPrint::build()
{
    m_state = &m_ccc->print;
    load_initial_data();

    auto page = add_options_page(L("Layers and perimeters"), "layers");

    auto optgroup = page->new_optgroup(L("Layers"));
    optgroup->append_single_option_line("layer_height");
    optgroup->append_single_option_line("faded_layers");

    page = add_options_page(L("Supports"), "support"/*"sla_supports"*/);

    optgroup = page->new_optgroup(L("Supports"));
    optgroup->append_single_option_line("supports_enable");
    optgroup->append_single_option_line("support_tree_type");
    optgroup->append_single_option_line("support_enforcers_only");
    
    build_sla_support_params({{"", L("Default")}, {"branching", L("Branching")}}, page);

    optgroup = page->new_optgroup(L("Automatic generation"));
    optgroup->append_single_option_line("support_points_density_relative");
    optgroup->append_single_option_line("support_points_minimal_distance");

    page = add_options_page(L("Pad"), "pad");
    optgroup = page->new_optgroup(L("Pad"));
    optgroup->append_single_option_line("pad_enable");
    optgroup->append_single_option_line("pad_wall_thickness");
    optgroup->append_single_option_line("pad_wall_height");
    optgroup->append_single_option_line("pad_brim_size");
    optgroup->append_single_option_line("pad_max_merge_distance");
    // TODO: Disabling this parameter for the beta release
//    optgroup->append_single_option_line("pad_edge_radius");
    optgroup->append_single_option_line("pad_wall_slope");

    optgroup->append_single_option_line("pad_around_object");
    optgroup->append_single_option_line("pad_around_object_everywhere");
    optgroup->append_single_option_line("pad_object_gap");
    optgroup->append_single_option_line("pad_object_connector_stride");
    optgroup->append_single_option_line("pad_object_connector_width");
    optgroup->append_single_option_line("pad_object_connector_penetration");
    
    page = add_options_page(L("Hollowing"), "hollowing");
    optgroup = page->new_optgroup(L("Hollowing"));
    optgroup->append_single_option_line("hollowing_enable");
    optgroup->append_single_option_line("hollowing_min_thickness");
    optgroup->append_single_option_line("hollowing_quality");
    optgroup->append_single_option_line("hollowing_closing_distance");

    page = add_options_page(L("Advanced"), "wrench");
    optgroup = page->new_optgroup(L("Slicing"));
    optgroup->append_single_option_line("slice_closing_radius");
    optgroup->append_single_option_line("slicing_mode");

    page = add_options_page(L("Output options"), "output+page_white");
    optgroup = page->new_optgroup(L("Output file"));
    Option option = optgroup->get_option("output_filename_format");
    option.opt.full_width = true;
    optgroup->append_single_option_line(option);

    page = add_options_page(L("Dependencies"), "wrench");
    optgroup = page->new_optgroup(L("Profile dependencies"));

    create_line_with_widget(optgroup.get(), "compatible_printers", "", [this](wxWindow* parent) {
        return compatible_widget_create(parent, m_compatible_printers);
    });

    option = optgroup->get_option("compatible_printers_condition");
    option.opt.full_width = true;
    optgroup->append_single_option_line(option);

    build_preset_description_line(optgroup.get());
}

void EditorSLAPrint::update_description_lines()
{
    AbstractEditor::update_description_lines();

    if (m_active_page && m_active_page->title() == "Supports")
    {
        bool is_visible = m_config->def()->get("support_object_elevation")->mode <= m_mode;
        if (m_support_object_elevation_description_line)
        {
            m_support_object_elevation_description_line->Show(is_visible);
            if (is_visible)
            {
                bool elev = !m_config->opt_bool("pad_enable") || !m_config->opt_bool("pad_around_object");
                m_support_object_elevation_description_line->SetText(elev ? "" :
                    WX::format_wxstr(_L("\"%1%\" is disabled because \"%2%\" is on in \"%3%\" category.\n"
                        "To enable \"%1%\", please switch off \"%2%\"")
                        , _L("Object elevation"), _L("Pad around object"), _L("Pad")));
            }
        }
    }
}

void EditorSLAPrint::toggle_options()
{
    if (m_active_page)
        m_config_manipulation.toggle_print_sla_options(m_config);
}

void EditorSLAPrint::update()
{
    m_update_cnt++;

    update_description_lines();
    Layout();

    m_update_cnt--;

    if (m_update_cnt == 0) {
        toggle_options();
/*  //!
        // update() could be called during undo/redo execution
        // Update of objectList can cause a crash in this case (because m_objects doesn't match ObjectList) 
        if (!wxGetApp().plater()->inside_snapshot_capture())
            wxGetApp().obj_list()->update_and_show_object_settings_item();

        wxGetApp().mainframe->on_config_changed(m_config);
*/
    }
}

void EditorSLAPrint::clear_pages()
{
    AbstractEditor::clear_pages();

    m_support_object_elevation_description_line = nullptr;
}

} 
