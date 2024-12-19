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

#include "EditorFilament.hpp"
#include "../Config/OptionsGroup.hpp"
//#include "WipeTowerDialog.hpp"

#include "Slic3r/Biz/Preset/PresetHints.hpp"

#include "Slic3r/App/WX/StringConversions.hpp"
#include "Slic3r/App/WX/I18N.hpp"
#include "Slic3r/App/WX/Widgets/CheckBox.hpp"

#include <wx/sizer.h>
#include <wx/string.h>

#include <wx/bmpbuttn.h>
#include <wx/wupdlock.h>

namespace Slic3r::App::Desktop::Preset {

using namespace WX;

const std::string& EditorFilament::get_custom_gcode(const t_config_option_key& opt_key)
{
    return config().opt_string(opt_key, unsigned(0));
}

void EditorFilament::set_custom_gcode(const t_config_option_key& opt_key, const std::string& value)
{
    std::vector<std::string> gcodes = static_cast<const ConfigOptionStrings*>(config().option(opt_key))->values;
    gcodes[0] = value;

    DynamicPrintConfig new_conf = config();
    new_conf.set_key_value(opt_key, new ConfigOptionStrings(gcodes));
    load_config(new_conf);
}

void EditorFilament::create_line_with_near_label_widget(ConfigOptionsGroupShp optgroup, const std::string& opt_key, int opt_index/* = 0*/)
{
    Line line {"",""};
    if (opt_key == "filament_retract_lift_above" || opt_key == "filament_retract_lift_below") {
        Option opt = optgroup->get_option(opt_key);
        opt.opt.label = opt.opt.full_label;
        line = optgroup->create_single_option_line(opt);
    }
    else
        line = optgroup->create_single_option_line(optgroup->get_option(opt_key));

    line.near_label_widget = [this, optgroup_wk = ConfigOptionsGroupWkp(optgroup), opt_key, opt_index](wxWindow* parent) {
        wxWindow* check_box = CheckBox::GetNewWin(parent);
        WX::w_config()->UpdateDarkUI(check_box);

        check_box->Bind(wxEVT_CHECKBOX, [optgroup_wk, opt_key, opt_index](wxCommandEvent& evt) {
            const bool is_checked = evt.IsChecked();
            if (auto optgroup_sh = optgroup_wk.lock(); optgroup_sh) {
                if (Field *field = optgroup_sh->get_fieldc(opt_key, opt_index); field != nullptr) {
                    field->toggle(is_checked);
                    if (is_checked)
                        field->set_last_meaningful_value();
                    else
                        field->set_na_value();
                }
            }
        });

        m_overrides_options[opt_key] = check_box;
        return check_box;
    };

    optgroup->append_line(line);
}

void EditorFilament::update_line_with_near_label_widget(ConfigOptionsGroupShp optgroup, const std::string& opt_key, int opt_index/* = 0*/, bool is_checked/* = true*/)
{
    if (!m_overrides_options[opt_key])
        return;
    m_overrides_options[opt_key]->Enable(is_checked);

    is_checked &= !config().option(opt_key)->is_nil();
    CheckBox::SetValue(m_overrides_options[opt_key], is_checked);

    Field* field = optgroup->get_fieldc(opt_key, opt_index);
    if (field != nullptr)
        field->toggle(is_checked);
}

std::vector<std::pair<std::string, std::vector<std::string>>> filament_overrides_option_keys {
    {"Travel lift", {
        "filament_retract_lift",
        "filament_travel_ramping_lift",
        "filament_travel_max_lift",
        "filament_travel_slope",
        "filament_travel_lift_before_obstacle",
        "filament_retract_lift_above",
        "filament_retract_lift_below"
    }},
    {"Retraction", {
        "filament_retract_length",
        "filament_retract_speed",
        "filament_deretract_speed",
        "filament_retract_restart_extra",
        "filament_retract_before_travel",
        "filament_retract_layer_change",
        "filament_wipe",
        "filament_retract_before_wipe",
    }},
    {"Retraction when tool is disabled", {
        "filament_retract_length_toolchange",
        "filament_retract_restart_extra_toolchange"
    }},
    {"Seams", {
        "filament_seam_gap_distance"
    }}
};

void EditorFilament::add_filament_overrides_page()
{
    PageShp page = add_options_page(L("Filament Overrides"), "wrench");

    const int extruder_idx = 0; // #ys_FIXME

    for (const auto&[title, keys] : filament_overrides_option_keys) {
        ConfigOptionsGroupShp optgroup = page->new_optgroup(L(title));
        for (const std::string& opt_key : keys) {
            create_line_with_near_label_widget(optgroup, opt_key, extruder_idx);
        }
    }
}

void EditorFilament::update_filament_overrides_page()
{
    if (!m_active_page || m_active_page->title() != "Filament Overrides")
        return;
    Page* page = m_active_page;


    const int extruder_idx = 0; // #ys_FIXME

    const bool have_retract_length = (
        config().option("filament_retract_length")->is_nil()
        || config().opt_float("filament_retract_length", extruder_idx) > 0
    );

    const bool uses_ramping_lift = (
        config().option("filament_travel_ramping_lift")->is_nil()
        || config().opt_bool("filament_travel_ramping_lift", extruder_idx)
    );

    const bool is_lifting =  (
        config().option("filament_travel_max_lift")->is_nil()
        || config().opt_float("filament_travel_max_lift", extruder_idx) > 0
        || config().option("filament_retract_lift")->is_nil()
        || config().opt_float("filament_retract_lift", extruder_idx) > 0
    );

    for (const auto&[title, keys] : filament_overrides_option_keys) {
        std::optional<ConfigOptionsGroupShp> optgroup{get_option_group(page, title)};
        if (!optgroup) {
            continue;
        }

        for (const std::string& opt_key : keys) {
            bool is_checked{true};
            if (
                title == "Retraction"
                && opt_key != "filament_retract_length"
                && !have_retract_length
            ) {
                is_checked = false;
            }

            if (
                title == "Travel lift"
                && uses_ramping_lift
                && opt_key == "filament_retract_lift"
                && !config().option("filament_travel_ramping_lift")->is_nil()
                && config().opt_bool("filament_travel_ramping_lift", extruder_idx)
            ) {
                is_checked = false;
            }

            if (
                title == "Travel lift"
                && !is_lifting
                && (
                    opt_key == "filament_retract_lift_above"
                    || opt_key == "filament_retract_lift_below"
                )
            ) {
                is_checked = false;
            }

            if (
                title == "Travel lift"
                && !uses_ramping_lift
                && opt_key != "filament_travel_ramping_lift"
                && opt_key != "filament_retract_lift"
                && opt_key != "filament_retract_lift_above"
                && opt_key != "filament_retract_lift_below"
            ) {
                is_checked = false;
            }

            update_line_with_near_label_widget(*optgroup, opt_key, extruder_idx, is_checked);
        }
    }
}

EditorFilament::EditorFilament(wxWindow* parent, Biz::Preset::PresetInteractor& preset_interactor) :
    AbstractEditor(parent, L("Filaments"), Slic3r::Preset::TYPE_FILAMENT, preset_interactor)
{
    m_config_interactor = std::make_unique<Biz::Preset::PresetConfigInteractor>(m_preset_interactor, Slic3r::Preset::TYPE_FILAMENT, 0);
}

void EditorFilament::build()
{
    load_initial_data();

    auto page = add_options_page(L("Filament"), "spool");
        auto optgroup = page->new_optgroup(L("Filament"));
        optgroup->append_single_option_line("filament_colour");
        optgroup->append_single_option_line("filament_diameter");
        optgroup->append_single_option_line("extrusion_multiplier");
        optgroup->append_single_option_line("filament_density");
        optgroup->append_single_option_line("filament_cost");
        optgroup->append_single_option_line("filament_spool_weight");

        optgroup->on_change = [this](t_config_option_key opt_key, boost::any value)
        {
            update_dirty();
            if (opt_key == "filament_spool_weight") {
                // Change of this option influences for an update of "Sliced Info"
//!                wxGetApp().sidebar().update_sliced_info_sizer();
//!                wxGetApp().sidebar().Layout();
            }
            else
                on_value_change(opt_key, value);
        };

        optgroup = page->new_optgroup(L("Temperature"));

        create_line_with_near_label_widget(optgroup, "idle_temperature");

        Line line = { L("Nozzle"), "" };
        line.append_option(optgroup->get_option("first_layer_temperature"));
        line.append_option(optgroup->get_option("temperature"));
        optgroup->append_line(line);

        line = { L("Bed"), "" };
        line.append_option(optgroup->get_option("first_layer_bed_temperature"));
        line.append_option(optgroup->get_option("bed_temperature"));
        optgroup->append_line(line);

        line = { L("Chamber"), "" };
        line.append_option(optgroup->get_option("chamber_temperature"));
        line.append_option(optgroup->get_option("chamber_minimal_temperature"));
        optgroup->append_line(line);

    page = add_options_page(L("Cooling"), "cooling");
        std::string category_path = "cooling_127569#";
        optgroup = page->new_optgroup(L("Enable"));
        optgroup->append_single_option_line("fan_always_on");
        optgroup->append_single_option_line("cooling");

        line = { "", "" };
        line.full_width = 1;
        line.widget = [this](wxWindow* parent) {
            return description_line_widget(parent, &m_cooling_description_line);
        };
        optgroup->append_line(line);

        optgroup = page->new_optgroup(L("Fan settings"));
        line = { L("Fan speed"), "" };
        line.label_path = category_path + "fan-settings";
        line.append_option(optgroup->get_option("min_fan_speed"));
        line.append_option(optgroup->get_option("max_fan_speed"));
        optgroup->append_line(line);

        optgroup->append_single_option_line("bridge_fan_speed", category_path + "fan-settings");
        optgroup->append_single_option_line("disable_fan_first_layers", category_path + "fan-settings");
        optgroup->append_single_option_line("full_fan_speed_layer", category_path + "fan-settings");

        optgroup = page->new_optgroup(L("Dynamic fan speeds"), 25);
        optgroup->append_single_option_line("enable_dynamic_fan_speeds", category_path + "dynamic-fan-speeds");
        optgroup->append_single_option_line("overhang_fan_speed_0", category_path + "dynamic-fan-speeds");
        optgroup->append_single_option_line("overhang_fan_speed_1", category_path + "dynamic-fan-speeds");
        optgroup->append_single_option_line("overhang_fan_speed_2", category_path + "dynamic-fan-speeds");
        optgroup->append_single_option_line("overhang_fan_speed_3", category_path + "dynamic-fan-speeds");

        optgroup = page->new_optgroup(L("Cooling thresholds"), 25);
        optgroup->append_single_option_line("fan_below_layer_time", category_path + "cooling-thresholds");
        optgroup->append_single_option_line("slowdown_below_layer_time", category_path + "cooling-thresholds");
        optgroup->append_single_option_line("min_print_speed", category_path + "cooling-thresholds");

    page = add_options_page(L("Advanced"), "wrench");
        optgroup = page->new_optgroup(L("Filament properties"));
        // Set size as all another fields for a better alignment
        Option option = optgroup->get_option("filament_type");
        option.opt.width = Field::def_width();
        optgroup->append_single_option_line(option);
        optgroup->append_single_option_line("filament_soluble");

        optgroup = page->new_optgroup(L("Print speed override"));
        optgroup->append_single_option_line("filament_max_volumetric_speed", "max-volumetric-speed_127176");

        line = { "", "" };
        line.full_width = 1;
        line.widget = [this](wxWindow* parent) {
            return description_line_widget(parent, &m_volumetric_speed_description_line);
        };
        optgroup->append_line(line);

        optgroup->append_single_option_line("filament_infill_max_speed", "max-simple-infill-speed");
        optgroup->append_single_option_line("filament_infill_max_crossing_speed", "max-crossing-infill-speed");

        optgroup = page->new_optgroup(L("Shrinkage compensation"));
        optgroup->append_single_option_line("filament_shrinkage_compensation_xy");
        optgroup->append_single_option_line("filament_shrinkage_compensation_z");

        optgroup = page->new_optgroup(L("Wipe tower parameters"));
        optgroup->append_single_option_line("filament_minimal_purge_on_wipe_tower");

        optgroup = page->new_optgroup(L("Toolchange parameters with single extruder MM printers"));
        optgroup->append_single_option_line("filament_loading_speed_start");
        optgroup->append_single_option_line("filament_loading_speed");
        optgroup->append_single_option_line("filament_unloading_speed_start");
        optgroup->append_single_option_line("filament_unloading_speed");
        optgroup->append_single_option_line("filament_load_time");
        optgroup->append_single_option_line("filament_unload_time");
        optgroup->append_single_option_line("filament_toolchange_delay");
        optgroup->append_single_option_line("filament_cooling_moves");
        optgroup->append_single_option_line("filament_cooling_initial_speed");
        optgroup->append_single_option_line("filament_cooling_final_speed");
        optgroup->append_single_option_line("filament_stamping_loading_speed");
        optgroup->append_single_option_line("filament_stamping_distance");
        optgroup->append_single_option_line("filament_purge_multiplier");

        create_line_with_widget(optgroup.get(), "filament_ramming_parameters", "", [this](wxWindow* parent) {
            auto ramming_dialog_btn = new wxButton(parent, wxID_ANY, _L("Ramming settings")+WX::dots, wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
            WX::w_config()->SetWindowVariantForButton(ramming_dialog_btn);
            WX::w_config()->UpdateDarkUI(ramming_dialog_btn);
            ramming_dialog_btn->SetFont(WX::w_config()->normal_font());
            ramming_dialog_btn->SetSize(ramming_dialog_btn->GetBestSize());
            auto sizer = new wxBoxSizer(wxHORIZONTAL);
            sizer->Add(ramming_dialog_btn);

            ramming_dialog_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& e) {
/*  //!
                RammingDialog dlg(this,(config().option<ConfigOptionStrings>("filament_ramming_parameters"))->get_at(0));
                if (dlg.ShowModal() == wxID_OK) {
                    load_key_value("filament_ramming_parameters", dlg.get_parameters());
                    update_changed_ui();
                }
*/
            });
            return sizer;
        });


        optgroup = page->new_optgroup(L("Toolchange parameters with multi extruder MM printers"));
        optgroup->append_single_option_line("filament_multitool_ramming");
        optgroup->append_single_option_line("filament_multitool_ramming_volume");
        optgroup->append_single_option_line("filament_multitool_ramming_flow");


    add_filament_overrides_page();


        const int gcode_field_height = 15; // 150
        const int notes_field_height = 25; // 250

    page = add_options_page(L("Custom G-code"), "cog");
        optgroup = page->new_optgroup(L("Start G-code"), 0);
        optgroup->on_change = [this, &optgroup_title = optgroup->title](const t_config_option_key& opt_key, const boost::any& value) {
            validate_custom_gcode_cb(optgroup_title, opt_key, value);
        };
        optgroup->edit_custom_gcode = [this](const t_config_option_key& opt_key) { edit_custom_gcode(opt_key); };
        option = optgroup->get_option("start_filament_gcode");
        option.opt.full_width = true;
        option.opt.is_code = true;
        option.opt.height = gcode_field_height;// 150;
        optgroup->append_single_option_line(option);

        optgroup = page->new_optgroup(L("End G-code"), 0);
        optgroup->on_change = [this, &optgroup_title = optgroup->title](const t_config_option_key& opt_key, const boost::any& value) {
            validate_custom_gcode_cb(optgroup_title, opt_key, value);
        };
        optgroup->edit_custom_gcode = [this](const t_config_option_key& opt_key) { edit_custom_gcode(opt_key); };
        option = optgroup->get_option("end_filament_gcode");
        option.opt.full_width = true;
        option.opt.is_code = true;
        option.opt.height = gcode_field_height;// 150;
        optgroup->append_single_option_line(option);

    page = add_options_page(L("Notes"), "note");
        optgroup = page->new_optgroup(L("Notes"), 0);
        optgroup->label_width = 0;
        option = optgroup->get_option("filament_notes");
        option.opt.full_width = true;
        option.opt.height = notes_field_height;// 250;
        optgroup->append_single_option_line(option);

    page = add_options_page(L("Dependencies"), "wrench");
        optgroup = page->new_optgroup(L("Profile dependencies"));
        create_line_with_widget(optgroup.get(), "compatible_printers", "", [this](wxWindow* parent) {
            return compatible_widget_create(parent, m_compatible_printers);
        });

        option = optgroup->get_option("compatible_printers_condition");
        option.opt.full_width = true;
        optgroup->append_single_option_line(option);

        create_line_with_widget(optgroup.get(), "compatible_prints", "", [this](wxWindow* parent) {
            return compatible_widget_create(parent, m_compatible_prints);
        });

        option = optgroup->get_option("compatible_prints_condition");
        option.opt.full_width = true;
        optgroup->append_single_option_line(option);

        build_preset_description_line(optgroup.get());
}

void EditorFilament::update_volumetric_flow_preset_hints()
{
    wxString text;
    try {
        text = WX::from_u8(Biz::Preset::PresetHints::maximum_volumetric_flow_description(m_preset_interactor.selected_config_container_context()));
    } catch (std::exception &ex) {
        text = _L("Volumetric flow hints not available") + "\n\n" + WX::from_u8(ex.what());
    }
    m_volumetric_speed_description_line->SetText(text);
}

void EditorFilament::update_description_lines()
{
    AbstractEditor::update_description_lines();

    if (!m_active_page)
        return;

    if (m_active_page->title() == "Cooling" && m_cooling_description_line)
        m_cooling_description_line->SetText(WX::from_u8(Biz::Preset::PresetHints::cooling_description(m_config_interactor->preset_state().edited_preset)));
    if (m_active_page->title() == "Advanced" && m_volumetric_speed_description_line)
        this->update_volumetric_flow_preset_hints();
}

void EditorFilament::toggle_options()
{
    if (!m_active_page)
        return;

    if (m_active_page->title() == "Cooling")
    {
        bool cooling = config().opt_bool("cooling", 0);
        bool fan_always_on = cooling || config().opt_bool("fan_always_on", 0);

        for (auto el : { "max_fan_speed", "fan_below_layer_time", "slowdown_below_layer_time", "min_print_speed" })
            toggle_option(el, cooling);

        for (auto el : { "min_fan_speed", "disable_fan_first_layers", "full_fan_speed_layer" })
            toggle_option(el, fan_always_on);

        bool dynamic_fan_speeds = config().opt_bool("enable_dynamic_fan_speeds", 0);
        for (int i = 0; i < 4; i++) {
        toggle_option("overhang_fan_speed_"+std::to_string(i),dynamic_fan_speeds);
        }
    }

    if (m_active_page->title() == "Advanced")
    {
        bool multitool_ramming = config().opt_bool("filament_multitool_ramming", 0);
        toggle_option("filament_multitool_ramming_volume", multitool_ramming);
        toggle_option("filament_multitool_ramming_flow", multitool_ramming);
    }

    if (m_active_page->title() == "Filament Overrides")
        update_filament_overrides_page();

    if (m_active_page->title() == "Filament") {
        Page* page = m_active_page;

        const auto og_it = std::find_if(page->optgroups.begin(), page->optgroups.end(), [](const ConfigOptionsGroupShp og) { return og->title == "Temperature"; });
        if (og_it != page->optgroups.end())
            update_line_with_near_label_widget(*og_it, "idle_temperature");
    }
}

void EditorFilament::update()
{
    m_update_cnt++;

    update_description_lines();
    Layout();

    toggle_options();

    m_update_cnt--;
/*  //!
    if (m_update_cnt == 0 && wxGetApp().mainframe)
        wxGetApp().mainframe->on_config_changed(m_config);
    */
}

void EditorFilament::clear_pages()
{
    AbstractEditor::clear_pages();

    m_volumetric_speed_description_line = nullptr;
	m_cooling_description_line = nullptr;

    for (auto& over_opt : m_overrides_options)
        over_opt.second = nullptr;
}

void EditorFilament::msw_rescale()
{
    for (const auto& over_opt : m_overrides_options)
        if (wxWindow* win = over_opt.second)
            win->SetInitialSize(win->GetBestSize());
    AbstractEditor::msw_rescale();
}

void EditorFilament::sys_color_changed()
{
    AbstractEditor::sys_color_changed();

    for (const auto& over_opt : m_overrides_options)
        if (wxWindow* check_box = over_opt.second) {
            WX::w_config()->UpdateDarkUI(check_box);
            CheckBox::SysColorChanged(check_box);
        }
}

} 
