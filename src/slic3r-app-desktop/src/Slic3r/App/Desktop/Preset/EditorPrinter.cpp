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


#include "EditorPrinter.hpp"
#include "../Config/OptionsGroup.hpp"

#include "libslic3r/GCode/GCodeWriter.hpp"
#include "libslic3r/GCode/Thumbnails.hpp"

#include "Slic3r/App/WX/StringConversions.hpp"
#include "Slic3r/App/WX/MsgDialog.hpp"
#include "Slic3r/App/WX/format.hpp"
#include "Slic3r/App/WX/I18N.hpp"

//#include "Search.hpp"

#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/string.h>

#include <wx/bmpbuttn.h>
#include <wx/wupdlock.h>

//!#include "Plater.hpp"        for -> SuppressBackgroundProcessingUpdate

//#include "slic3r/GUI/BedShapeDialog.hpp"

namespace Slic3r::App::Desktop::Preset {

using WX::_L;

EditorPrinter::EditorPrinter(wxWindow* parent, Biz::Preset::PresetInteractor& preset_interactor) :
    AbstractEditor(parent, _L("Printers"), Slic3r::Preset::TYPE_PRINTER, preset_interactor)
{
    m_config_interactor = std::make_unique<Biz::Preset::PresetConfigInteractor>(preset_interactor, Slic3r::Preset::TYPE_PRINTER, 0);
}

void EditorPrinter::init_options_list()
{
    AbstractEditor::init_options_list();

    if (printer_technology == ptFFF)
        m_options_list.emplace("extruders_count", m_opt_status_value);
}

void EditorPrinter::build()
{
    printer_technology = m_preset_interactor.selected_config_container_context().printer_technology();
/*      //! some better solution?
    // For DiffPresetDialog we use options list which is saved in Searcher class.
    // Options for the Searcher is added in the moment of pages creation.
    // So, build first of all printer pages for non-selected printer technology...
    std::string def_preset_name = "- default " + std::string(printer_technology == ptSLA ? "FFF" : "SLA") + " -";
    m_config = &m_presets->find_preset(def_preset_name)->config;
    printer_technology == ptSLA ? build_fff() : build_sla();
    if (printer_technology == ptSLA)
        m_extruders_count_old = 0;// revert this value 
*/
    // ... and than for selected printer technology
    load_initial_data();
    printer_technology == ptSLA ? build_sla() : build_fff();
}

void EditorPrinter::build_print_host_upload_group(Page* page) //! maybe it's a time to delete this one?
{
    ConfigOptionsGroupShp optgroup = page->new_optgroup(_L("Print Host upload"));

    wxString description_line_text = _L(""
        "Note: All parameters from this group are moved to the Physical Printer settings (see changelog).\n\n"
        "A new Physical Printer profile is created by clicking on the \"cog\" icon right of the Printer profiles combo box, "
        "by selecting the \"Add physical printer\" item in the Printer combo box. The Physical Printer profile editor opens "
        "also when clicking on the \"cog\" icon in the Printer settings tab. The Physical Printer profiles are being stored "
        "into PrusaSlicer/physical_printer directory.");

    Line line = {{}, {} };
    line.full_width = 1;
    line.widget = [this, description_line_text](wxWindow* parent) {
        return description_line_widget(parent, m_config_interactor->preset_state().selected_preset->printer_technology() == ptFFF ?
                                       &m_fff_print_host_upload_description_line : &m_sla_print_host_upload_description_line,
                                       description_line_text);
    };
    optgroup->append_line(line);
}

static wxString get_info_klipper_string()
{
    return _L("Emitting machine limits to G-code is not supported with Klipper G-code flavor.\n"
              "The option was switched to \"Use for time estimate\".");
}

void EditorPrinter::build_fff()
{
    if (!m_pages.empty())
        m_pages.resize(0);
    // to avoid redundant memory allocation / deallocation during extruders count changing
    m_pages.reserve(30);

    auto   *nozzle_diameter = dynamic_cast<const ConfigOptionFloats*>(config().option("nozzle_diameter"));
    m_initial_extruders_count = m_extruders_count = nozzle_diameter->values.size();
//!    wxGetApp().sidebar().update_objects_list_extruder_column(m_initial_extruders_count);

    const Slic3r::Preset* parent_preset = printer_technology == ptSLA ? nullptr // just for first build, if SLA printer preset is selected 
                                  : m_config_interactor->preset_state().selected_preset_parent;
    m_sys_extruders_count = parent_preset == nullptr ? 0 :
            static_cast<const ConfigOptionFloats*>(parent_preset->config.option("nozzle_diameter"))->values.size();

    auto page = add_options_page(_L("General"), "printer");
        auto optgroup = page->new_optgroup(_L("Size and coordinates"));

        create_line_with_widget(optgroup.get(), "bed_shape", "custom-svg-and-png-bed-textures_124612", [this](wxWindow* parent) {
            return 	create_bed_shape_widget(parent);
        });

        optgroup->append_single_option_line("max_print_height");
        optgroup->append_single_option_line("z_offset");

        optgroup = page->new_optgroup(_L("Capabilities"));
        ConfigOptionDef def;
            def.type =  coInt,
            def.set_default_value(new ConfigOptionInt(1));
            def.label = L("Extruders");
            def.tooltip = L("Number of extruders of the printer.");
            def.min = 1;
            def.max = 256;
            def.mode = comExpert;
        Option option(def, "extruders_count");
        optgroup->append_single_option_line(option);
        optgroup->append_single_option_line("single_extruder_multi_material");

        optgroup->on_change = [this, optgroup_wk = ConfigOptionsGroupWkp(optgroup)](t_config_option_key opt_key, boost::any value) {
            auto optgroup_sh = optgroup_wk.lock();
            if (!optgroup_sh)
                return;

            // optgroup->get_value() return int for def.type == coInt,
            // Thus, there should be boost::any_cast<int> !
            // Otherwise, boost::any_cast<size_t> causes an "unhandled unknown exception"
            size_t extruders_count = size_t(boost::any_cast<int>(optgroup_sh->get_value("extruders_count")));
            wxTheApp->CallAfter([this, opt_key, value, extruders_count]() {
                if (opt_key == "extruders_count" || opt_key == "single_extruder_multi_material") {
                    extruders_count_changed(extruders_count);
                    init_options_list(); // m_options_list should be updated before UI updating
                    update_dirty();
                    if (opt_key == "single_extruder_multi_material") { // the single_extruder_multimaterial was added to force pages
                        on_value_change(opt_key, value);                      // rebuild - let's make sure the on_value_change is not skipped

                        if (boost::any_cast<bool>(value) && m_extruders_count > 1) {
                            //!SuppressBackgroundProcessingUpdate sbpu;
                            std::vector<double> nozzle_diameters = static_cast<const ConfigOptionFloats*>(config().option("nozzle_diameter"))->values;
                            std::vector<unsigned char> high_flow_nozzles = static_cast<const ConfigOptionBools*>(config().option("nozzle_high_flow"))->values;
                            assert(nozzle_diameters.size() == high_flow_nozzles.size());

                            for (size_t i = 1; i < nozzle_diameters.size(); ++i) {
                                // if value is differs from first nozzle diameter value
                                if (fabs(nozzle_diameters[i] - nozzle_diameters[0]) > EPSILON || high_flow_nozzles[i] != high_flow_nozzles[0]) {
                                    const wxString msg_text = _L("This is a single extruder multimaterial printer, \n"
                                        "all extruders must have the same nozzle diameter and 'High flow' state.\n"
                                        "Do you want to change these values for all extruders to first extruder values?");
                                    WX::MessageDialog dialog(parent(), msg_text, _L("Extruder settings do not match"), wxICON_WARNING | wxYES_NO);

                                    DynamicPrintConfig new_conf = config();
                                    if (dialog.ShowModal() == wxID_YES) {
                                        for (size_t i = 1; i < nozzle_diameters.size(); i++) {
                                            nozzle_diameters[i] = nozzle_diameters[0];
                                            high_flow_nozzles[i] = high_flow_nozzles[0];
                                        }

                                        new_conf.set_key_value("nozzle_diameter", new ConfigOptionFloats(nozzle_diameters));
                                        new_conf.set_key_value("nozzle_high_flow", new ConfigOptionBools(high_flow_nozzles));
                                    }
                                    else
                                        new_conf.set_key_value("single_extruder_multi_material", new ConfigOptionBool(false));

                                    load_config(new_conf);
                                    break;
                                }
                            }
                        }

//!                        m_preset_bundle->update_compatible(PresetSelectCompatibleType::Never);
                        /*  //!
                        // Upadte related comboboxes on Sidebar and Tabs
                        Sidebar& sidebar = wxGetApp().plater()->sidebar();
                        for (const Slic3r::Preset::Type& type : {Slic3r::Preset::TYPE_PRINT, Slic3r::Preset::TYPE_FILAMENT}) {
                            sidebar.update_presets(type);
                            wxGetApp().get_tab(type)->update_tab_ui();
                        }
*/
                    }
                }
                else {
                    update_dirty();
                    on_value_change(opt_key, value);
                }
            });
        };

        build_print_host_upload_group(page.get());

        optgroup = page->new_optgroup(_L("Firmware"));
        optgroup->append_single_option_line("gcode_flavor");

        option = optgroup->get_option("thumbnails");
        option.opt.full_width = true;
        optgroup->append_single_option_line(option);

        optgroup->append_single_option_line("silent_mode");
        optgroup->append_single_option_line("remaining_times");
        optgroup->append_single_option_line("binary_gcode");

        optgroup->on_change = [this](t_config_option_key opt_key, boost::any value) {
            wxTheApp->CallAfter([this, opt_key, value]() {
                if (opt_key == "thumbnails" && config().has("thumbnails_format")) {
                    // to backward compatibility we need to update "thumbnails_format" from new "thumbnails"
                    if (const std::string val = boost::any_cast<std::string>(value); !value.empty()) {
                        auto [thumbnails_list, errors] = GCodeThumbnails::make_and_check_thumbnail_list(val);

                        if (errors != enum_bitmask<ThumbnailError>()) {
                            // TRN: First argument is parameter name, the second one is the value.
                            std::string error_str = format(_u8L("Invalid value provided for parameter %1%: %2%"), "thumbnails", val);
                            error_str += GCodeThumbnails::get_error_string(errors);
                            WX::InfoDialog(parent(), _L("G-code flavor is switched"), WX::from_u8(error_str)).ShowModal();
                        }

                        if (!thumbnails_list.empty()) {
                            GCodeThumbnailsFormat old_format = GCodeThumbnailsFormat(config().option("thumbnails_format")->getInt());
                            GCodeThumbnailsFormat new_format = thumbnails_list.begin()->first;
                            if (old_format != new_format) {
                                DynamicPrintConfig new_conf = config();

                                auto* opt = config().option("thumbnails_format")->clone();
                                opt->setInt(int(new_format));
                                new_conf.set_key_value("thumbnails_format", opt);

                                load_config(new_conf);
                            }
                        }
                    }
                }
                if (opt_key == "silent_mode") {
                    bool val = boost::any_cast<bool>(value);
                    if (m_use_silent_mode != val) {
                        m_rebuild_kinematics_page = true;
                        m_use_silent_mode = val;
                    }
                }
                if (opt_key == "gcode_flavor") {
                    const GCodeFlavor flavor = static_cast<GCodeFlavor>(boost::any_cast<int>(value));
                    bool supports_travel_acceleration = GCodeWriter::supports_separate_travel_acceleration(flavor);
                    bool supports_min_feedrates       = (flavor == gcfMarlinFirmware || flavor == gcfMarlinLegacy);
                    if (supports_travel_acceleration != m_supports_travel_acceleration || supports_min_feedrates != m_supports_min_feedrates) {
                        m_rebuild_kinematics_page = true;
                        m_supports_travel_acceleration = supports_travel_acceleration;
                        m_supports_min_feedrates = supports_min_feedrates;
                    }

                    const bool is_emit_to_gcode = config().option("machine_limits_usage")->getInt() == static_cast<int>(MachineLimitsUsage::EmitToGCode);
                    if ((flavor == gcfKlipper && is_emit_to_gcode) || (!m_supports_min_feedrates && m_use_silent_mode)) {
                        DynamicPrintConfig new_conf = config();
                        wxString msg;

                        if (flavor == gcfKlipper && is_emit_to_gcode) {
                            msg = get_info_klipper_string();

                            auto machine_limits_usage = static_cast<ConfigOptionEnum<MachineLimitsUsage>*>(config().option("machine_limits_usage")->clone());
                            machine_limits_usage->value = MachineLimitsUsage::TimeEstimateOnly;
                            new_conf.set_key_value("machine_limits_usage", machine_limits_usage);
                        }

                        if (!m_supports_min_feedrates && m_use_silent_mode) {
                            if (!msg.IsEmpty())
                                msg += WX::from_u8("\n\n");
                            msg += _L("The selected G-code flavor does not support the machine limitation for Stealth mode.\n"
                                      "Stealth mode will not be applied and will be disabled.");

                            auto silent_mode = static_cast<ConfigOptionBool*>(config().option("silent_mode")->clone());
                            silent_mode->value = false;
                            new_conf.set_key_value("silent_mode", silent_mode);
                        }

                        WX::InfoDialog(parent(), _L("G-code flavor is switched"), msg).ShowModal();
                        load_config(new_conf);
                    }
                }
                build_unregular_pages();
                update_dirty();
                on_value_change(opt_key, value);
            });
        };

        optgroup = page->new_optgroup(_L("Advanced"));
        optgroup->append_single_option_line("use_relative_e_distances");
        optgroup->append_single_option_line("use_firmware_retraction");
        optgroup->append_single_option_line("use_volumetric_e");
        optgroup->append_single_option_line("variable_layer_height");
        optgroup->append_single_option_line("prefer_clockwise_movements");

    const int gcode_field_height = 15; // 150
    const int notes_field_height = 25; // 250
    page = add_options_page(_L("Custom G-code"), "cog");
        optgroup = page->new_optgroup(_L("Start G-code"), 0);
        optgroup->on_change = [this, &optgroup_title = optgroup->title](const t_config_option_key& opt_key, const boost::any& value) {
            validate_custom_gcode_cb(optgroup_title, opt_key, value);
        };
        optgroup->edit_custom_gcode = [this](const t_config_option_key& opt_key) { edit_custom_gcode(opt_key); };
        option = optgroup->get_option("start_gcode");
        option.opt.full_width = true;
        option.opt.is_code = true;
        option.opt.height = 3 * gcode_field_height;//150;
        optgroup->append_single_option_line(option);

        optgroup = page->new_optgroup(_L("Start G-Code options"));
        optgroup->append_single_option_line("autoemit_temperature_commands");

        optgroup = page->new_optgroup(_L("End G-code"), 0);
        optgroup->on_change = [this, &optgroup_title = optgroup->title](const t_config_option_key& opt_key, const boost::any& value) {
            validate_custom_gcode_cb(optgroup_title, opt_key, value);
        };
        optgroup->edit_custom_gcode = [this](const t_config_option_key& opt_key) { edit_custom_gcode(opt_key); };
        option = optgroup->get_option("end_gcode");
        option.opt.full_width = true;
        option.opt.is_code = true;
        option.opt.height = 1.75 * gcode_field_height;//150;
        optgroup->append_single_option_line(option);

        optgroup = page->new_optgroup(_L("Before layer change G-code"), 0);
        optgroup->on_change = [this, &optgroup_title = optgroup->title](const t_config_option_key& opt_key, const boost::any& value) {
            validate_custom_gcode_cb(optgroup_title, opt_key, value);
        };
        optgroup->edit_custom_gcode = [this](const t_config_option_key& opt_key) { edit_custom_gcode(opt_key); };
        option = optgroup->get_option("before_layer_gcode");
        option.opt.full_width = true;
        option.opt.is_code = true;
        option.opt.height = gcode_field_height;//150;
        optgroup->append_single_option_line(option);

        optgroup = page->new_optgroup(_L("After layer change G-code"), 0);
        optgroup->on_change = [this, &optgroup_title = optgroup->title](const t_config_option_key& opt_key, const boost::any& value) {
            validate_custom_gcode_cb(optgroup_title, opt_key, value);
        };
        optgroup->edit_custom_gcode = [this](const t_config_option_key& opt_key) { edit_custom_gcode(opt_key); };
        option = optgroup->get_option("layer_gcode");
        option.opt.full_width = true;
        option.opt.is_code = true;
        option.opt.height = gcode_field_height;//150;
        optgroup->append_single_option_line(option);

        optgroup = page->new_optgroup(_L("Tool change G-code"), 0);
        optgroup->on_change = [this, &optgroup_title = optgroup->title](const t_config_option_key& opt_key, const boost::any& value) {
            validate_custom_gcode_cb(optgroup_title, opt_key, value);
        };
        optgroup->edit_custom_gcode = [this](const t_config_option_key& opt_key) { edit_custom_gcode(opt_key); };
        option = optgroup->get_option("toolchange_gcode");
        option.opt.full_width = true;
        option.opt.is_code = true;
        option.opt.height = gcode_field_height;//150;
        optgroup->append_single_option_line(option);

        optgroup = page->new_optgroup(_L("Between objects G-code (for sequential printing)"), 0);
        optgroup->on_change = [this, &optgroup_title = optgroup->title](const t_config_option_key& opt_key, const boost::any& value) {
            validate_custom_gcode_cb(optgroup_title, opt_key, value);
        };
        optgroup->edit_custom_gcode = [this](const t_config_option_key& opt_key) { edit_custom_gcode(opt_key); };
        option = optgroup->get_option("between_objects_gcode");
        option.opt.full_width = true;
        option.opt.is_code = true;
        option.opt.height = gcode_field_height;//150;
        optgroup->append_single_option_line(option);

        optgroup = page->new_optgroup(_L("Color Change G-code"), 0);
        optgroup->on_change = [this, &optgroup_title = optgroup->title](const t_config_option_key& opt_key, const boost::any& value) {
            validate_custom_gcode_cb(optgroup_title, opt_key, value);
        };
        optgroup->edit_custom_gcode = [this](const t_config_option_key& opt_key) { edit_custom_gcode(opt_key); };
        option = optgroup->get_option("color_change_gcode");
        option.opt.is_code = true;
        option.opt.height = gcode_field_height;//150;
        optgroup->append_single_option_line(option);

        optgroup = page->new_optgroup(_L("Pause Print G-code"), 0);
        optgroup->on_change = [this, &optgroup_title = optgroup->title](const t_config_option_key& opt_key, const boost::any& value) {
            validate_custom_gcode_cb(optgroup_title, opt_key, value);
        };
        optgroup->edit_custom_gcode = [this](const t_config_option_key& opt_key) { edit_custom_gcode(opt_key); };
        option = optgroup->get_option("pause_print_gcode");
        option.opt.is_code = true;
        option.opt.height = gcode_field_height;//150;
        optgroup->append_single_option_line(option);

        optgroup = page->new_optgroup(_L("Template Custom G-code"), 0);
        optgroup->on_change = [this, &optgroup_title = optgroup->title](const t_config_option_key& opt_key, const boost::any& value) {
            validate_custom_gcode_cb(optgroup_title, opt_key, value);
        };
        optgroup->edit_custom_gcode = [this](const t_config_option_key& opt_key) { edit_custom_gcode(opt_key); };
        option = optgroup->get_option("template_custom_gcode");
        option.opt.is_code = true;
        option.opt.height = gcode_field_height;//150;
        optgroup->append_single_option_line(option);

    page = add_options_page(_L("Notes"), "note");
        optgroup = page->new_optgroup(_L("Notes"), 0);
        option = optgroup->get_option("printer_notes");
        option.opt.full_width = true;
        option.opt.height = notes_field_height;//250;
        optgroup->append_single_option_line(option);

    page = add_options_page(_L("Dependencies"), "wrench");
        optgroup = page->new_optgroup(_L("Profile dependencies"));

        build_preset_description_line(optgroup.get());

    build_unregular_pages(true);
}

void EditorPrinter::build_sla()
{
    if (!m_pages.empty())
        m_pages.resize(0);
    auto page = add_options_page(_L("General"), "printer");
    auto optgroup = page->new_optgroup(_L("Size and coordinates"));

    create_line_with_widget(optgroup.get(), "bed_shape", "custom-svg-and-png-bed-textures_124612", [this](wxWindow* parent) {
        return 	create_bed_shape_widget(parent);
    });
    optgroup->append_single_option_line("max_print_height");

    optgroup = page->new_optgroup(_L("Display"));
    optgroup->append_single_option_line("display_width");
    optgroup->append_single_option_line("display_height");

    auto option = optgroup->get_option("display_pixels_x");
    Line line = { WX::from_u8(option.opt.full_label), {} };
    line.append_option(option);
    line.append_option(optgroup->get_option("display_pixels_y"));
    optgroup->append_line(line);
    optgroup->append_single_option_line("display_orientation");

    // FIXME: This should be on one line in the UI
    optgroup->append_single_option_line("display_mirror_x");
    optgroup->append_single_option_line("display_mirror_y");

    optgroup = page->new_optgroup(_L("Tilt"));
    line = { _L("Tilt time"), {} };
    line.append_option(optgroup->get_option("fast_tilt_time"));
    line.append_option(optgroup->get_option("slow_tilt_time"));
    line.append_option(optgroup->get_option("high_viscosity_tilt_time"));
    optgroup->append_line(line);
//    optgroup->append_single_option_line("area_fill");

    optgroup = page->new_optgroup(_L("Corrections"));
    line = Line{ WX::from_u8(config().def()->get("relative_correction")->full_label), {} };
    for (auto& axis : { "X", "Y", "Z" }) {
        auto opt = optgroup->get_option(std::string("relative_correction_") + char(std::tolower(axis[0])));
        opt.opt.label = axis;
        line.append_option(opt);
    }
    optgroup->append_line(line);
    optgroup->append_single_option_line("absolute_correction");
    optgroup->append_single_option_line("elefant_foot_compensation");
    optgroup->append_single_option_line("elefant_foot_min_width");
    optgroup->append_single_option_line("gamma_correction");
    
    optgroup = page->new_optgroup(_L("Exposure"));
    optgroup->append_single_option_line("min_exposure_time");
    optgroup->append_single_option_line("max_exposure_time");
    optgroup->append_single_option_line("min_initial_exposure_time");
    optgroup->append_single_option_line("max_initial_exposure_time");


    optgroup = page->new_optgroup(_L("Output"));
    optgroup->append_single_option_line("sla_archive_format");
    optgroup->append_single_option_line("sla_output_precision");

    build_print_host_upload_group(page.get());

    const int notes_field_height = 25; // 250

    page = add_options_page(_L("Notes"), "note");
    optgroup = page->new_optgroup(_L("Notes"), 0);
    option = optgroup->get_option("printer_notes");
    option.opt.full_width = true;
    option.opt.height = notes_field_height;//250;
    optgroup->append_single_option_line(option);

    page = add_options_page(_L("Dependencies"), "wrench");
    optgroup = page->new_optgroup(_L("Profile dependencies"));

    build_preset_description_line(optgroup.get());
}

void EditorPrinter::extruders_count_changed(size_t extruders_count)
{
    bool is_count_changed = false;
    bool is_updated_mm_filament_presets = false;
    if (m_extruders_count != extruders_count) {
        m_extruders_count = extruders_count;
        m_config_interactor->set_config_num_extruders(extruders_count);
        is_count_changed = is_updated_mm_filament_presets = true;
    }
    /*  //!
    else if (m_extruders_count == 1 &&
             m_preset_bundle->project_config.option<ConfigOptionFloats>("wiping_volumes_matrix")->values.size()>1) {
        is_updated_mm_filament_presets = true;
    }

    if (is_updated_mm_filament_presets) {
        m_preset_bundle->update_multi_material_filament_presets();
        m_preset_bundle->update_filaments_compatible(PresetSelectCompatibleType::OnlyIfWasCompatible);
    }
*/
    /* This function should be call in any case because of correct updating/rebuilding
     * of unregular pages of a Printer Settings
     */
    build_unregular_pages();

    if (is_count_changed) {
        on_value_change("extruders_count", extruders_count);
//!        wxGetApp().sidebar().update_objects_list_extruder_column(extruders_count);
    }
}

void EditorPrinter::append_option_line(ConfigOptionsGroupShp optgroup, const std::string opt_key)
{
    auto option = optgroup->get_option(opt_key, 0);
    auto line = Line{ WX::from_u8(option.opt.full_label), {} };
    line.append_option(option);
    if (m_use_silent_mode 
        || printer_technology == ptSLA // just for first build, if SLA printer preset is selected 
        )
        line.append_option(optgroup->get_option(opt_key, 1));
    optgroup->append_line(line);
}

PageShp EditorPrinter::build_kinematics_page()
{
    auto page = add_options_page(_L("Machine limits"), "cog", true);

    auto optgroup = page->new_optgroup(_L("General"));
    {
	    optgroup->append_single_option_line("machine_limits_usage");
        Line line { {}, {} };
        line.full_width = 1;
        line.widget = [this](wxWindow* parent) {
            return description_line_widget(parent, &m_machine_limits_description_line);
        };
        optgroup->append_line(line);
    }

    optgroup->on_change = [this](const t_config_option_key& opt_key, boost::any value)
    {
        if (opt_key == "machine_limits_usage" &&
            static_cast<MachineLimitsUsage>(boost::any_cast<int>(value)) == MachineLimitsUsage::EmitToGCode &&
            static_cast<GCodeFlavor>(config().option("gcode_flavor")->getInt()) == gcfKlipper)
        {
            DynamicPrintConfig new_conf = config();

            auto machine_limits_usage = static_cast<ConfigOptionEnum<MachineLimitsUsage>*>(config().option("machine_limits_usage")->clone());
            machine_limits_usage->value = MachineLimitsUsage::TimeEstimateOnly;

            new_conf.set_key_value("machine_limits_usage", machine_limits_usage);

            WX::InfoDialog(parent(), wxEmptyString, get_info_klipper_string()).ShowModal();
            load_config(new_conf);
        }

        update_dirty();
        update();
    };

    if (m_use_silent_mode) {
        std::vector<std::pair<std::string, std::string>> legend_columns = {
            {L("Normal"), L("Values in this column are for Normal mode")},
            {L("Stealth"), L("Values in this column are for Stealth mode")}
        };

        create_legend(page, legend_columns, comAdvanced);
    }

    const std::vector<std::string> axes{ "x", "y", "z", "e" };
    optgroup = page->new_optgroup(_L("Maximum feedrates"));
        for (const std::string &axis : axes)	{
            append_option_line(optgroup, "machine_max_feedrate_" + axis);
        }

    optgroup = page->new_optgroup(_L("Maximum accelerations"));
        for (const std::string &axis : axes)	{
            append_option_line(optgroup, "machine_max_acceleration_" + axis);
        }
        append_option_line(optgroup, "machine_max_acceleration_extruding");
        append_option_line(optgroup, "machine_max_acceleration_retracting");
        if (m_supports_travel_acceleration)
            append_option_line(optgroup, "machine_max_acceleration_travel");

    optgroup = page->new_optgroup(_L("Jerk limits"));
        for (const std::string &axis : axes)	{
            append_option_line(optgroup, "machine_max_jerk_" + axis);
        }

        if (m_supports_min_feedrates) {
            optgroup = page->new_optgroup(_L("Minimum feedrates"));
            append_option_line(optgroup, "machine_min_extruding_rate");
            append_option_line(optgroup, "machine_min_travel_rate");
        }

    return page;
}

const std::vector<std::string> extruder_options = {
    "min_layer_height", "max_layer_height", "extruder_offset",
    "retract_length", "retract_lift", "retract_lift_above", "retract_lift_below",
    "retract_speed", "deretract_speed", "retract_restart_extra", "retract_before_travel",
    "retract_layer_change", "wipe", "retract_before_wipe", "travel_ramping_lift",
    "travel_slope", "travel_max_lift", "travel_lift_before_obstacle", "nozzle_high_flow",
    "retract_length_toolchange", "retract_restart_extra_toolchange",
};

void EditorPrinter::build_extruder_pages(size_t n_before_extruders)
{
    for (auto extruder_idx = m_extruders_count_old; extruder_idx < m_extruders_count; ++extruder_idx) {
        //# build page
        const wxString&page_name = wxString::Format(WX::from_u8("Extruder %d"), int(extruder_idx + 1));
        auto           page      = add_options_page(page_name, "funnel", true);
        m_pages.insert(m_pages.begin() + n_before_extruders + extruder_idx, page);

        auto optgroup = page->new_optgroup(_L("Size"));
        optgroup->append_single_option_line("nozzle_diameter", "", extruder_idx);

        optgroup->on_change = [this, extruder_idx](const t_config_option_key&opt_key, boost::any value)
        {
            const bool is_single_extruder_MM = config().opt_bool("single_extruder_multi_material");
            const bool is_nozzle_diameter_changed = opt_key.find("nozzle_diameter") != std::string::npos;
            const bool is_high_flow_changed = opt_key.find("nozzle_high_flow") != std::string::npos;

            if (is_single_extruder_MM && m_extruders_count > 1 && is_nozzle_diameter_changed)
            {
                //!SuppressBackgroundProcessingUpdate sbpu;
                const double new_nd = boost::any_cast<double>(value);
                std::vector<double> nozzle_diameters = static_cast<const ConfigOptionFloats*>(config().option("nozzle_diameter"))->values;

                // if value was changed
                if (fabs(nozzle_diameters[extruder_idx == 0 ? 1 : 0] - new_nd) > EPSILON)
                {
                    const wxString msg_text = _L("This is a single extruder multimaterial printer, diameters of all extruders "
                                                 "will be set to the new value. Do you want to proceed?");
                    WX::MessageDialog dialog(parent(), msg_text, _L("Nozzle diameter"), wxICON_WARNING | wxYES_NO);

                    DynamicPrintConfig new_conf = config();
                    if (dialog.ShowModal() == wxID_YES) {
                        for (size_t i = 0; i < nozzle_diameters.size(); i++) {
                            if (i==extruder_idx)
                                continue;
                            nozzle_diameters[i] = new_nd;
                        }
                    }
                    else
                        nozzle_diameters[extruder_idx] = nozzle_diameters[extruder_idx == 0 ? 1 : 0];

                    new_conf.set_key_value("nozzle_diameter", new ConfigOptionFloats(nozzle_diameters));
                    load_config(new_conf);
                }
            }
/*  //!
            if (is_single_extruder_MM && m_extruders_count > 1 && is_high_flow_changed)
            {
                SuppressBackgroundProcessingUpdate sbpu;
                const unsigned char new_hf = boost::any_cast<unsigned char>(value);
                std::vector<unsigned char> nozzle_high_flow = static_cast<const ConfigOptionBools*>(m_config->option("nozzle_high_flow"))->values;

                // if value was changed
                if (nozzle_high_flow[extruder_idx == 0 ? 1 : 0] != new_hf)
                {
                    const wxString msg_text = _L("This is a single extruder multimaterial printer, 'high_flow' state of all extruders "
                                                 "will be set to the new value. Do you want to proceed?");
                    MessageDialog dialog(parent(), msg_text, _L("Extruder settings do not match"), wxICON_WARNING | wxYES_NO);

                    DynamicPrintConfig new_conf = *m_config;
                    if (dialog.ShowModal() == wxID_YES) {
                        for (size_t i = 0; i < nozzle_high_flow.size(); i++) {
                            if (i==extruder_idx)
                                continue;
                            nozzle_high_flow[i] = new_hf;
                        }
                    }
                    else
                        nozzle_high_flow[extruder_idx] = nozzle_high_flow[extruder_idx == 0 ? 1 : 0];

                    new_conf.set_key_value("nozzle_high_flow", new ConfigOptionBools(nozzle_high_flow));
                    load_config(new_conf);
                }
            }

            if (is_nozzle_diameter_changed || is_high_flow_changed) {
                if (extruder_idx == 0)
                    // Mark the print & filament enabled if they are compatible with the currently selected preset.
                    // If saving the preset changes compatibility with other presets, keep the now incompatible dependent presets selected, however with a "red flag" icon showing that they are no more compatible.
                    m_preset_bundle->update_compatible(PresetSelectCompatibleType::Never);
                else
                    m_preset_bundle->update_filaments_compatible(PresetSelectCompatibleType::Never, extruder_idx);
            }
*/
            update_dirty();
            update();
        };

        optgroup->append_single_option_line("nozzle_high_flow", "", extruder_idx);

        optgroup = page->new_optgroup(_L("Preview"));

        auto reset_to_filament_color = [this, extruder_idx](wxWindow*parent) {
            WX::ScalableButton* btn = new WX::ScalableButton(parent, wxID_ANY, "undo", _L("Reset to Filament Color"),
                                                     wxDefaultSize, wxDefaultPosition, wxBU_LEFT | wxBU_EXACTFIT);
            btn->SetFont(WX::w_config()->normal_font());
            btn->SetSize(btn->GetBestSize());
            auto sizer = new wxBoxSizer(wxHORIZONTAL);
            sizer->Add(btn);

            btn->Bind(wxEVT_BUTTON, [this, extruder_idx](wxCommandEvent&e)
            {
                std::vector<std::string> colors = static_cast<const ConfigOptionStrings*>(config().option("extruder_colour"))->values;
                colors[extruder_idx]            = "";

                DynamicPrintConfig new_conf = config();
                new_conf.set_key_value("extruder_colour", new ConfigOptionStrings(colors));
                load_config(new_conf);

                update_dirty();
                update();
            });

            parent->Bind(wxEVT_UPDATE_UI, [this, extruder_idx](wxUpdateUIEvent& evt) {
                evt.Enable(!static_cast<const ConfigOptionStrings*>(config().option("extruder_colour"))->values[extruder_idx].empty());
            }, btn->GetId());

            return sizer;
        };
        Line line = optgroup->create_single_option_line("extruder_colour", "", extruder_idx);
        line.append_widget(reset_to_filament_color);
        optgroup->append_line(line);

        optgroup = page->new_optgroup({});

        auto copy_settings_btn = 
        line            = { {}, {}};
        line.full_width = 1;
        line.widget = [this, extruder_idx](wxWindow* parent) {
            WX::ScalableButton* btn = new WX::ScalableButton(parent, wxID_ANY, "copy", _L("Apply below setting to other extruders"),
                                                     wxDefaultSize, wxDefaultPosition, wxBU_LEFT | wxBU_EXACTFIT);
            auto sizer = new wxBoxSizer(wxHORIZONTAL);
            sizer->Add(btn);

            btn->Bind(wxEVT_BUTTON, [this, extruder_idx](wxCommandEvent& e) {
                DynamicPrintConfig new_conf = config();

                for (const std::string& opt : extruder_options) {
                    const ConfigOption* other_opt = config().option(opt);
                    for (size_t extruder = 0; extruder < m_extruders_count; ++extruder) {
                        if (extruder == extruder_idx)
                            continue;
                        static_cast<ConfigOptionVectorBase*>(new_conf.option(opt, false))->set_at(other_opt, extruder, extruder_idx);
                    }
                }
                load_config(new_conf);

                update_dirty();
                update();
            });

            auto has_changes = [this]() {
                auto dirty_options = m_config_interactor->preset_state().current_dirty_options(true);
#if 1
                dirty_options.erase(std::remove_if(dirty_options.begin(), dirty_options.end(), 
                    [](const std::string& opt) { return opt.find("extruder_colour") != std::string::npos || opt.find("nozzle_diameter") != std::string::npos; }), dirty_options.end());
                return !dirty_options.empty();
#else
                // if we wont to apply enable status for each extruder separately
                for (const std::string& opt : extruder_options)
                    if (std::find(dirty_options.begin(), dirty_options.end(), opt+"#"+std::to_string(extruder_idx)) != dirty_options.end())
                        return true;
                return false;
#endif
            };

            parent->Bind(wxEVT_UPDATE_UI, [this, has_changes](wxUpdateUIEvent& evt) {
                evt.Enable(m_extruders_count > 1 && has_changes());
            }, btn->GetId());

            return sizer;
        };
        optgroup->append_line(line);

        optgroup = page->new_optgroup(_L("Layer height limits"));
        optgroup->append_single_option_line("min_layer_height", "", extruder_idx);
        optgroup->append_single_option_line("max_layer_height", "", extruder_idx);

        optgroup = page->new_optgroup(_L("Position (for multi-extruder printers)"));
        optgroup->append_single_option_line("extruder_offset", "", extruder_idx);

        optgroup = page->new_optgroup(_L("Travel lift"));
        optgroup->append_single_option_line("retract_lift", "", extruder_idx);
        optgroup->append_single_option_line("travel_ramping_lift", "", extruder_idx);
        optgroup->append_single_option_line("travel_max_lift", "", extruder_idx);
        optgroup->append_single_option_line("travel_slope", "", extruder_idx);
        optgroup->append_single_option_line("travel_lift_before_obstacle", "", extruder_idx);

        line = { _L("Only lift"), {} };
        line.append_option(optgroup->get_option("retract_lift_above", extruder_idx));
        line.append_option(optgroup->get_option("retract_lift_below", extruder_idx));
        optgroup->append_line(line);

        optgroup = page->new_optgroup(_L("Retraction"));
        optgroup->append_single_option_line("retract_length", "", extruder_idx);
        optgroup->append_single_option_line("retract_speed", "", extruder_idx);
        optgroup->append_single_option_line("deretract_speed", "", extruder_idx);
        optgroup->append_single_option_line("retract_restart_extra", "", extruder_idx);
        optgroup->append_single_option_line("retract_before_travel", "", extruder_idx);
        optgroup->append_single_option_line("retract_layer_change", "", extruder_idx);
        optgroup->append_single_option_line("wipe", "", extruder_idx);
        optgroup->append_single_option_line("retract_before_wipe", "", extruder_idx);

        optgroup = page->new_optgroup(_L("Retraction when tool is disabled (advanced settings for multi-extruder setups)"));
        optgroup->append_single_option_line("retract_length_toolchange", "", extruder_idx);
        optgroup->append_single_option_line("retract_restart_extra_toolchange", "", extruder_idx);
    }

    // # remove extra pages
    if (m_extruders_count < m_extruders_count_old)
        m_pages.erase(	m_pages.begin() + n_before_extruders + m_extruders_count,
                        m_pages.begin() + n_before_extruders + m_extruders_count_old);
}

/* Previous name build_extruder_pages().
 *
 * This function was renamed because of now it implements not just an extruder pages building,
 * but "Machine limits" and "Single extruder MM setup" too
 * (These pages can changes according to the another values of a current preset)
 * */
void EditorPrinter::build_unregular_pages(bool from_initial_build/* = false*/)
{
    size_t		n_before_extruders = 2;			//	Count of pages before Extruder pages
    auto        flavor = config().option<ConfigOptionEnum<GCodeFlavor>>("gcode_flavor")->value;
    bool		show_mach_limits = (flavor == gcfMarlinLegacy || flavor == gcfMarlinFirmware || flavor == gcfRepRapFirmware || flavor == gcfKlipper);

    /* ! Freeze/Thaw in this function is needed to avoid call OnPaint() for erased pages
     * and be cause of application crash, when try to change Preset in moment,
     * when one of unregular pages is selected.
     *  */
    Freeze();

    // Add/delete Kinematics page according to show_mach_limits
    size_t existed_page = 0;
    for (size_t i = n_before_extruders; i < m_pages.size(); ++i) // first make sure it's not there already
        if (m_pages[i]->title().find(_L("Machine limits")) != std::string::npos) {
            if (!show_mach_limits || m_rebuild_kinematics_page)
                m_pages.erase(m_pages.begin() + i);
            else
                existed_page = i;
            break;
        }

    if (existed_page < n_before_extruders && (show_mach_limits || from_initial_build)) {
        auto page = build_kinematics_page();
        if (from_initial_build && !show_mach_limits)
            page->clear();
        else
            m_pages.insert(m_pages.begin() + n_before_extruders, page);
    }

    if (show_mach_limits)
        n_before_extruders++;
    size_t		n_after_single_extruder_MM = 2; //	Count of pages after single_extruder_multi_material page

    if (m_extruders_count_old == m_extruders_count ||
        (m_has_single_extruder_MM_page && m_extruders_count == 1))
    {
        // if we have a single extruder MM setup, add a page with configuration options:
        for (size_t i = 0; i < m_pages.size(); ++i) // first make sure it's not there already
            if (m_pages[i]->title().find(_L("Single extruder MM setup")) != std::string::npos) {
                m_pages.erase(m_pages.begin() + i);
                break;
            }
        m_has_single_extruder_MM_page = false;
    }
    if (from_initial_build ||
        (m_extruders_count > 1 && config().opt_bool("single_extruder_multi_material") && !m_has_single_extruder_MM_page)) {
        // create a page, but pretend it's an extruder page, so we can add it to m_pages ourselves
        auto page = add_options_page(_L("Single extruder MM setup"), "printer", true);
        auto optgroup = page->new_optgroup(_L("Single extruder multimaterial parameters"));
        optgroup->append_single_option_line("cooling_tube_retraction");
        optgroup->append_single_option_line("cooling_tube_length");
        optgroup->append_single_option_line("parking_pos_retraction");
        optgroup->append_single_option_line("extra_loading_move");
        optgroup->append_single_option_line("multimaterial_purging");
        optgroup->append_single_option_line("high_current_on_filament_swap");
        if (from_initial_build)
            page->clear();
        else {
            m_pages.insert(m_pages.end() - n_after_single_extruder_MM, page);
            m_has_single_extruder_MM_page = true;
        }
    }

    // Build missed extruder pages
    build_extruder_pages(n_before_extruders);

    Thaw();

    m_extruders_count_old = m_extruders_count;

    if (from_initial_build && printer_technology == ptSLA)
        return; // next part of code is no needed to execute at this moment

    rebuild_page_tree();

    // Reload preset pages with current configuration values
    reload_config();
}

// this gets executed after preset is loaded and before GUI fields are updated
void EditorPrinter::on_preset_loaded()
{
    // update the extruders count field
    auto   *nozzle_diameter = dynamic_cast<const ConfigOptionFloats*>(config().option("nozzle_diameter"));
    size_t extruders_count = nozzle_diameter->values.size();
    // update the GUI field according to the number of nozzle diameters supplied
    extruders_count_changed(extruders_count);
}

void EditorPrinter::update_pages()
{
    // update m_pages ONLY if printer technology is changed
    const PrinterTechnology new_printer_technology = m_config_interactor->preset_state().edited_preset.printer_technology();
    if (new_printer_technology == printer_technology)
        return;

    //clear all active pages before switching
    clear_pages();

    // set m_pages to m_pages_(technology before changing)
    printer_technology == ptFFF ? m_pages.swap(m_pages_fff) : m_pages.swap(m_pages_sla);

    // build Tab according to the technology, if it's not exist jet OR
    // set m_pages_(technology after changing) to m_pages
    // printer_technology will be set by Tab::load_current_preset()
    if (new_printer_technology == ptFFF)
    {
        if (m_pages_fff.empty())
        {
            build_fff();
            if (m_extruders_count > 1)
            {
//!                m_preset_bundle->update_multi_material_filament_presets();
//!                m_preset_bundle->update_filaments_compatible(PresetSelectCompatibleType::OnlyIfWasCompatible);
                on_value_change("extruders_count", m_extruders_count);
            }
        }
        else
            m_pages.swap(m_pages_fff);

//!         wxGetApp().sidebar().update_objects_list_extruder_column(m_extruders_count);
    }
    else
        m_pages_sla.empty() ? build_sla() : m_pages.swap(m_pages_sla);

    rebuild_page_tree();
}

void EditorPrinter::reload_config()
{
    AbstractEditor::reload_config();

    // "extruders_count" doesn't update from the update_config(),
    // so update it implicitly
    if (m_active_page && m_active_page->title() == WX::from_u8("General"))
        m_active_page->set_value("extruders_count", int(m_extruders_count));
}

void EditorPrinter::activate_selected_page(std::function<void()> throw_if_canceled)
{
    AbstractEditor::activate_selected_page(throw_if_canceled);

    // "extruders_count" doesn't update from the update_config(),
    // so update it implicitly
    if (m_active_page && m_active_page->title() == WX::from_u8("General"))
        m_active_page->set_value("extruders_count", int(m_extruders_count));
}

void EditorPrinter::clear_pages()
{
    AbstractEditor::clear_pages();

    m_machine_limits_description_line           = nullptr;
    m_fff_print_host_upload_description_line    = nullptr;
    m_sla_print_host_upload_description_line    = nullptr;
}

void EditorPrinter::toggle_options()
{
    if (!m_active_page || m_config_interactor->preset_state().edited_preset.printer_technology() == ptSLA)
        return;

    const GCodeFlavor flavor = config().option<ConfigOptionEnum<GCodeFlavor>>("gcode_flavor")->value;
    bool have_multiple_extruders = m_extruders_count > 1;
    if (m_active_page->title() == WX::from_u8("Custom G-code"))
        toggle_option("toolchange_gcode", have_multiple_extruders);
    if (m_active_page->title() == WX::from_u8("General")) {
        toggle_option("single_extruder_multi_material", have_multiple_extruders);

        bool is_marlin_flavor = flavor == gcfMarlinLegacy || flavor == gcfMarlinFirmware;
        // Disable silent mode for non-marlin firmwares.
        toggle_option("silent_mode", is_marlin_flavor);
    }

    wxString extruder_number;
    long val;
    if (m_active_page->title().StartsWith(WX::from_u8("Extruder "), &extruder_number) && extruder_number.ToLong(&val) &&
        val > 0 && (size_t)val <= m_extruders_count)
    {
        size_t i = size_t(val - 1);
        bool have_retract_length = config().opt_float("retract_length", i) > 0;
        const bool ramping_lift = config().opt_bool("travel_ramping_lift", i);
        const bool lifts_z = (ramping_lift && config().opt_float("travel_max_lift", i) > 0)
                          || (! ramping_lift && config().opt_float("retract_lift", i) > 0);


        // when using firmware retraction, firmware decides retraction length
        bool use_firmware_retraction = config().opt_bool("use_firmware_retraction");
        toggle_option("retract_length", !use_firmware_retraction, i);

        toggle_option("retract_lift", ! ramping_lift, i);
        toggle_option("travel_max_lift", ramping_lift, i);
        toggle_option("travel_slope", ramping_lift, i);

        // user can customize travel length if we have retraction length or we"re using
        // firmware retraction
        toggle_option("retract_before_travel", have_retract_length || use_firmware_retraction, i);

        // user can customize other retraction options if retraction is enabled
        bool retraction = (have_retract_length || use_firmware_retraction);
        std::vector<std::string> vec = {  };
        for (auto el : vec)
            toggle_option("retract_layer_change", retraction, i);

        // retract lift above / below only applies if using retract lift
        vec.resize(0);
        vec = { "retract_lift_above", "retract_lift_below" };
        for (auto el : vec)
            toggle_option(el, lifts_z, i);

        // some options only apply when not using firmware retraction
        vec.resize(0);
        vec = { "retract_speed", "deretract_speed", "retract_before_wipe", "retract_restart_extra", "wipe" };
        for (auto el : vec)
            toggle_option(el, retraction && !use_firmware_retraction, i);

        bool wipe = config().opt_bool("wipe", i);
        toggle_option("retract_before_wipe", wipe, i);

        if (use_firmware_retraction && wipe) {
            WX::MessageDialog dialog(parent(),
                _L("The Wipe option is not available when using the Firmware Retraction mode.\n"
                    "\nShall I disable it in order to enable Firmware Retraction?"),
                _L("Firmware Retraction"), wxICON_WARNING | wxYES | wxNO);

            DynamicPrintConfig new_conf = config();
            if (dialog.ShowModal() == wxID_YES) {
                auto wipe = static_cast<ConfigOptionBools*>(config().option("wipe")->clone());
                for (size_t w = 0; w < wipe->values.size(); w++)
                    wipe->values[w] = false;
                new_conf.set_key_value("wipe", wipe);
            }
            else {
                new_conf.set_key_value("use_firmware_retraction", new ConfigOptionBool(false));
            }
            load_config(new_conf);
        }

        toggle_option("travel_lift_before_obstacle", ramping_lift, i);

        toggle_option("retract_length_toolchange", have_multiple_extruders, i);

        bool toolchange_retraction = config().opt_float("retract_length_toolchange", i) > 0;
        toggle_option("retract_restart_extra_toolchange", have_multiple_extruders && toolchange_retraction, i);
    }

    if (m_active_page->title() == WX::from_u8("Machine limits") && m_machine_limits_description_line) {
        assert(flavor == gcfMarlinLegacy
            || flavor == gcfMarlinFirmware
            || flavor == gcfRepRapFirmware
            || flavor == gcfKlipper);
		const auto *machine_limits_usage = config().option<ConfigOptionEnum<MachineLimitsUsage>>("machine_limits_usage");
		bool enabled = machine_limits_usage->value != MachineLimitsUsage::Ignore;
        bool silent_mode = config().opt_bool("silent_mode");
        int  max_field = silent_mode ? 2 : 1;
    	for (const std::string &opt : Slic3r::Preset::machine_limits_options())
            for (int i = 0; i < max_field; ++ i)
	            toggle_option(opt, enabled, i);
        update_machine_limits_description(machine_limits_usage->value);
    }
}

void EditorPrinter::update()
{
    m_update_cnt++;
    auto pt = m_preset_interactor.selected_config_container_context().printer_technology();
    pt == ptFFF ? update_fff() : update_sla();
    m_update_cnt--;

    update_description_lines();
    Layout();

//!    if (m_update_cnt == 0)
//!         wxGetApp().mainframe->on_config_changed(m_config);
}

void EditorPrinter::update_fff()
{
    if (m_use_silent_mode != config().opt_bool("silent_mode"))	{
        m_rebuild_kinematics_page = true;
        m_use_silent_mode = config().opt_bool("silent_mode");
    }

    const auto flavor = config().option<ConfigOptionEnum<GCodeFlavor>>("gcode_flavor")->value;
    bool supports_travel_acceleration = (flavor == gcfMarlinFirmware || flavor == gcfRepRapFirmware);
    bool supports_min_feedrates       = (flavor == gcfMarlinFirmware || flavor == gcfMarlinLegacy);
    if (m_supports_travel_acceleration != supports_travel_acceleration || m_supports_min_feedrates != supports_min_feedrates) {
        m_rebuild_kinematics_page = true;
        m_supports_travel_acceleration = supports_travel_acceleration;
        m_supports_min_feedrates = supports_min_feedrates;
    }

    toggle_options();
}

void EditorPrinter::update_sla()
{
}

// Return a callback to create a EditorPrinter widget to edit bed shape
wxSizer* EditorPrinter::create_bed_shape_widget(wxWindow* parent)
{
    WX::ScalableButton* btn = new WX::ScalableButton(parent, wxID_ANY, "printer", WX::from_u8(" ") + _L("Set") + WX::from_u8(" ") + WX::dots,
        wxDefaultSize, wxDefaultPosition, wxBU_LEFT | wxBU_EXACTFIT);
    btn->SetFont(WX::w_config()->normal_font());
    btn->SetSize(btn->GetBestSize());

    auto sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(btn, 0, wxALIGN_CENTER_VERTICAL);
/* //!
    btn->Bind(wxEVT_BUTTON, ([this](wxCommandEvent e)
        {
            BedShapeDialog dlg(this);
            dlg.build_dialog(*config().option<ConfigOptionPoints>("bed_shape"),
                *config().option<ConfigOptionString>("bed_custom_texture"),
                *config().option<ConfigOptionString>("bed_custom_model"));
            if (dlg.ShowModal() == wxID_OK) {
                const std::vector<Vec2d>& shape = dlg.get_shape();
                const std::string& custom_texture = dlg.get_custom_texture();
                const std::string& custom_model = dlg.get_custom_model();
                if (!shape.empty())
                {
                    load_key_value("bed_shape", shape);
                    load_key_value("bed_custom_texture", custom_texture);
                    load_key_value("bed_custom_model", custom_model);
                    update_changed_ui();
                }
            }
        }));

    // may be it is not a best place, but 
    // add information about Category/Grope for "bed_custom_texture" and "bed_custom_model" as a copy from "bed_shape" option
    {
        Search::OptionsSearcher& searcher = wxGetApp().searcher();
        const Search::GroupAndCategory& gc = searcher.get_group_and_category("bed_shape");
        searcher.add_key("bed_custom_texture", m_type, gc.group, gc.category);
        searcher.add_key("bed_custom_model", m_type, gc.group, gc.category);
    }
*/
    return sizer;
}

void EditorPrinter::cache_extruder_cnt(const DynamicPrintConfig* config/* = nullptr*/)
{
    const DynamicPrintConfig& cached_config = config ? *config : this->config();
    if (Slic3r::Preset::printer_technology(cached_config) == ptSLA)
        return;

    // get extruders count 
    auto* nozzle_diameter = dynamic_cast<const ConfigOptionFloats*>(cached_config.option("nozzle_diameter"));
    m_cache_extruder_count = nozzle_diameter->values.size(); //m_extruders_count;
}

bool EditorPrinter::apply_extruder_cnt_from_cache()
{
    if (m_config_interactor->preset_state().edited_preset.printer_technology() == ptSLA)
        return false;

    if (m_cache_extruder_count > 0) {
        m_config_interactor->set_config_num_extruders(m_cache_extruder_count);
        m_cache_extruder_count = 0;
        return true;
    }
    return false;
}

void EditorPrinter::update_sla_prusa_specific_visibility()
{
    if (m_active_page && m_active_page->title() == WX::from_u8("General")) {
        auto og_it = std::find_if(m_active_page->optgroups.begin(), m_active_page->optgroups.end(), [](const ConfigOptionsGroupShp og) { return og->title == WX::from_u8("Tilt"); });
        if (og_it != m_active_page->optgroups.end()) {            
            og_it->get()->Show(m_mode == comExpert && !is_prusa_printer());
            Layout();
        }
    }
}

void EditorPrinter::update_machine_limits_description(const MachineLimitsUsage usage)
{
	wxString text;
	switch (usage) {
	case MachineLimitsUsage::EmitToGCode:
		text = _L("Machine limits will be emitted to G-code and used to estimate print time.");
		break;
	case MachineLimitsUsage::TimeEstimateOnly:
		text = _L("Machine limits will NOT be emitted to G-code, however they will be used to estimate print time, "
			      "which may therefore not be accurate as the printer may apply a different set of machine limits.");
		break;
	case MachineLimitsUsage::Ignore:
		text = _L("Machine limits are not set, therefore the print time estimate may not be accurate.");
		break;
	default: assert(false);
	}
    m_machine_limits_description_line->SetText(text);
}

} 
