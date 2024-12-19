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

#include "EditorPrint.hpp"
#include "../Config/OptionsGroup.hpp"

#include "Slic3r/Biz/Preset/PresetHints.hpp"

#include "Slic3r/App/WX/StringConversions.hpp"
#include "Slic3r/App/WX/MsgDialog.hpp"
#include "Slic3r/App/WX/I18N.hpp"

#include <wx/sizer.h>
#include <wx/string.h>


namespace Slic3r::App::Desktop::Preset {

using WX::_L;

EditorPrint::EditorPrint(wxWindow* parent, Biz::Preset::PresetInteractor& preset_interactor) :
    AbstractEditor(parent, L("Print Settings"), Slic3r::Preset::TYPE_PRINT, preset_interactor)
{
    m_config_interactor = std::make_unique<Biz::Preset::PresetConfigInteractor>(preset_interactor, Slic3r::Preset::TYPE_PRINT, 0);
}

void EditorPrint::build()
{
    load_initial_data();

    auto page = add_options_page(L("Layers and perimeters"), "layers");
        std::string category_path = "layers-and-perimeters_1748#";
        auto optgroup = page->new_optgroup(L("Layer height"));
        optgroup->append_single_option_line("layer_height", category_path + "layer-height");
        optgroup->append_single_option_line("first_layer_height", category_path + "first-layer-height");

        optgroup = page->new_optgroup(L("Vertical shells"));
        optgroup->append_single_option_line("perimeters", category_path + "perimeters");
        optgroup->append_single_option_line("spiral_vase", category_path + "spiral-vase");

        Line line { "", "" };
        line.full_width = 1;
        line.label_path = category_path + "recommended-thin-wall-thickness";
        line.widget = [this](wxWindow* parent) {
            return description_line_widget(parent, &m_recommended_thin_wall_thickness_description_line);
        };
        optgroup->append_line(line);

        optgroup = page->new_optgroup(L("Horizontal shells"));
        line = { L("Solid layers"), "" };
        line.label_path = category_path + "solid-layers-top-bottom";
        line.append_option(optgroup->get_option("top_solid_layers"));
        line.append_option(optgroup->get_option("bottom_solid_layers"));
        optgroup->append_line(line);
    	line = { L("Minimum shell thickness"), "" };
        line.append_option(optgroup->get_option("top_solid_min_thickness"));
        line.append_option(optgroup->get_option("bottom_solid_min_thickness"));
        optgroup->append_line(line);
		line = { "", "" };
	    line.full_width = 1;
	    line.widget = [this](wxWindow* parent) {
	        return description_line_widget(parent, &m_top_bottom_shell_thickness_explanation);
	    };
	    optgroup->append_line(line);

        optgroup = page->new_optgroup(L("Quality (slower slicing)"));
        optgroup->append_single_option_line("extra_perimeters", category_path + "extra-perimeters-if-needed");
        optgroup->append_single_option_line("extra_perimeters_on_overhangs", category_path + "extra-perimeters-on-overhangs");
        optgroup->append_single_option_line("ensure_vertical_shell_thickness", category_path + "ensure-vertical-shell-thickness");
        optgroup->append_single_option_line("avoid_crossing_curled_overhangs", category_path + "avoid-crossing-curled-overhangs");
        optgroup->append_single_option_line("avoid_crossing_perimeters", category_path + "avoid-crossing-perimeters");
        optgroup->append_single_option_line("avoid_crossing_perimeters_max_detour", category_path + "avoid_crossing_perimeters_max_detour");
        optgroup->append_single_option_line("thin_walls", category_path + "detect-thin-walls");
        optgroup->append_single_option_line("thick_bridges", category_path + "thick_bridges");
        optgroup->append_single_option_line("overhangs", category_path + "detect-bridging-perimeters");

        optgroup = page->new_optgroup(L("Advanced"));
        optgroup->append_single_option_line("seam_position", category_path + "seam-position");
        optgroup->append_single_option_line("seam_gap_distance", category_path + "seam-gap-distance");
        optgroup->append_single_option_line("staggered_inner_seams", category_path + "staggered-inner-seams");

        optgroup->append_single_option_line("scarf_seam_placement", category_path + "scarf-seam-placement");
        optgroup->append_single_option_line("scarf_seam_only_on_smooth", category_path + "scarf-seam-only-on-smooth");
        optgroup->append_single_option_line("scarf_seam_start_height", category_path + "scarf-seam-start-height");
        optgroup->append_single_option_line("scarf_seam_entire_loop", category_path + "scarf-seam-entire-loop");
        optgroup->append_single_option_line("scarf_seam_length", category_path + "scarf-seam-length");
        optgroup->append_single_option_line("scarf_seam_max_segment_length", category_path + "scarf-seam-max-segment-length");
        optgroup->append_single_option_line("scarf_seam_on_inner_perimeters", category_path + "scarf-seam-on-inner-perimeters");

        optgroup->append_single_option_line("external_perimeters_first", category_path + "external-perimeters-first");
        optgroup->append_single_option_line("gap_fill_enabled", category_path + "fill-gaps");
        optgroup->append_single_option_line("perimeter_generator");

        optgroup = page->new_optgroup(L("Fuzzy skin (experimental)"));
        category_path = "fuzzy-skin_246186/#";
        optgroup->append_single_option_line("fuzzy_skin", category_path + "fuzzy-skin-type");
        optgroup->append_single_option_line("fuzzy_skin_thickness", category_path + "fuzzy-skin-thickness");
        optgroup->append_single_option_line("fuzzy_skin_point_dist", category_path + "fuzzy-skin-point-distance");

        optgroup = page->new_optgroup(L("Only one perimeter"));
        optgroup->append_single_option_line("top_one_perimeter_type", category_path + "top-one-perimeter-type");
        optgroup->append_single_option_line("only_one_perimeter_first_layer", category_path + "only-one-perimeter-first-layer");

    page = add_options_page(L("Infill"), "infill");
        category_path = "infill_42#";
        optgroup = page->new_optgroup(L("Infill"));
        optgroup->append_single_option_line("fill_density", category_path + "fill-density");
        optgroup->append_single_option_line("fill_pattern", category_path + "fill-pattern");
        optgroup->append_single_option_line("infill_anchor", category_path + "fill-pattern");
        optgroup->append_single_option_line("infill_anchor_max", category_path + "fill-pattern");
        optgroup->append_single_option_line("top_fill_pattern", category_path + "top-fill-pattern");
        optgroup->append_single_option_line("bottom_fill_pattern", category_path + "bottom-fill-pattern");

        optgroup = page->new_optgroup(L("Ironing"));
        category_path = "ironing_177488#";
        optgroup->append_single_option_line("ironing", category_path);
        optgroup->append_single_option_line("ironing_type", category_path + "ironing-type");
        optgroup->append_single_option_line("ironing_flowrate", category_path + "flow-rate");
        optgroup->append_single_option_line("ironing_spacing", category_path + "spacing-between-ironing-passes");

        optgroup = page->new_optgroup(L("Reducing printing time"));
        category_path = "infill_42#";
        optgroup->append_single_option_line("automatic_infill_combination");
        optgroup->append_single_option_line("automatic_infill_combination_max_layer_height");
        optgroup->append_single_option_line("infill_every_layers", category_path + "combine-infill-every-x-layers");

        optgroup = page->new_optgroup(L("Advanced"));
        optgroup->append_single_option_line("solid_infill_every_layers", category_path + "solid-infill-every-x-layers");
        optgroup->append_single_option_line("fill_angle", category_path + "fill-angle");
        optgroup->append_single_option_line("solid_infill_below_area", category_path + "solid-infill-threshold-area");
        optgroup->append_single_option_line("bridge_angle");
        optgroup->append_single_option_line("only_retract_when_crossing_perimeters");
        optgroup->append_single_option_line("infill_first");

    page = add_options_page(L("Skirt and brim"), "skirt+brim");
        category_path = "skirt-and-brim_133969#";
        optgroup = page->new_optgroup(L("Skirt"));
        optgroup->append_single_option_line("skirts", category_path + "skirt");
        optgroup->append_single_option_line("skirt_distance", category_path + "skirt");
        optgroup->append_single_option_line("skirt_height", category_path + "skirt");
        optgroup->append_single_option_line("draft_shield", category_path + "skirt");
        optgroup->append_single_option_line("min_skirt_length", category_path + "skirt");

        optgroup = page->new_optgroup(L("Brim"));
        optgroup->append_single_option_line("brim_type", category_path + "brim");
        optgroup->append_single_option_line("brim_width", category_path + "brim");
        optgroup->append_single_option_line("brim_separation", category_path + "brim");

    page = add_options_page(L("Support material"), "support");
        category_path = "support-material_1698#";
        optgroup = page->new_optgroup(L("Support material"));
        optgroup->append_single_option_line("support_material", category_path + "generate-support-material");
        optgroup->append_single_option_line("support_material_auto", category_path + "auto-generated-supports");
        optgroup->append_single_option_line("support_material_threshold", category_path + "overhang-threshold");
        optgroup->append_single_option_line("support_material_enforce_layers", category_path + "enforce-support-for-the-first");
        optgroup->append_single_option_line("raft_first_layer_density", category_path + "raft-first-layer-density");
        optgroup->append_single_option_line("raft_first_layer_expansion", category_path + "raft-first-layer-expansion");

        optgroup = page->new_optgroup(L("Raft"));
        optgroup->append_single_option_line("raft_layers", category_path + "raft-layers");
        optgroup->append_single_option_line("raft_contact_distance", category_path + "raft-layers");
        optgroup->append_single_option_line("raft_expansion");

        optgroup = page->new_optgroup(L("Options for support material and raft"));
        optgroup->append_single_option_line("support_material_style", category_path + "style");
        optgroup->append_single_option_line("support_material_contact_distance", category_path + "contact-z-distance");
        optgroup->append_single_option_line("support_material_bottom_contact_distance", category_path + "contact-z-distance");
        optgroup->append_single_option_line("support_material_pattern", category_path + "pattern");
        optgroup->append_single_option_line("support_material_with_sheath", category_path + "with-sheath-around-the-support");
        optgroup->append_single_option_line("support_material_spacing", category_path + "pattern-spacing-0-inf");
        optgroup->append_single_option_line("support_material_angle", category_path + "pattern-angle");
        optgroup->append_single_option_line("support_material_closing_radius", category_path + "pattern-angle");
        optgroup->append_single_option_line("support_material_interface_layers", category_path + "interface-layers");
        optgroup->append_single_option_line("support_material_bottom_interface_layers", category_path + "interface-layers");
        optgroup->append_single_option_line("support_material_interface_pattern", category_path + "interface-pattern");
        optgroup->append_single_option_line("support_material_interface_spacing", category_path + "interface-pattern-spacing");
        optgroup->append_single_option_line("support_material_interface_contact_loops", category_path + "interface-loops");
        optgroup->append_single_option_line("support_material_buildplate_only", category_path + "support-on-build-plate-only");
        optgroup->append_single_option_line("support_material_xy_spacing", category_path + "xy-separation-between-an-object-and-its-support");
        optgroup->append_single_option_line("dont_support_bridges", category_path + "dont-support-bridges");
        optgroup->append_single_option_line("support_material_synchronize_layers", category_path + "synchronize-with-object-layers");

        optgroup = page->new_optgroup(L("Organic supports"));
        const std::string path = "organic-supports_480131#organic-supports-settings";
        optgroup->append_single_option_line("support_tree_angle", path);
        optgroup->append_single_option_line("support_tree_angle_slow", path);
        optgroup->append_single_option_line("support_tree_branch_diameter", path);
        optgroup->append_single_option_line("support_tree_branch_diameter_angle", path);
        optgroup->append_single_option_line("support_tree_branch_diameter_double_wall", path);
        optgroup->append_single_option_line("support_tree_tip_diameter", path);
        optgroup->append_single_option_line("support_tree_branch_distance", path);
        optgroup->append_single_option_line("support_tree_top_rate", path);

    page = add_options_page(L("Speed"), "time");
        optgroup = page->new_optgroup(L("Speed for print moves"));
        optgroup->append_single_option_line("perimeter_speed");
        optgroup->append_single_option_line("small_perimeter_speed");
        optgroup->append_single_option_line("external_perimeter_speed");
        optgroup->append_single_option_line("infill_speed");
        optgroup->append_single_option_line("solid_infill_speed");
        optgroup->append_single_option_line("top_solid_infill_speed");
        optgroup->append_single_option_line("support_material_speed");
        optgroup->append_single_option_line("support_material_interface_speed");
        optgroup->append_single_option_line("bridge_speed");
        optgroup->append_single_option_line("gap_fill_speed");
        optgroup->append_single_option_line("ironing_speed");

        optgroup = page->new_optgroup(L("Dynamic overhang speed"));
        optgroup->append_single_option_line("enable_dynamic_overhang_speeds");
        optgroup->append_single_option_line("overhang_speed_0");
        optgroup->append_single_option_line("overhang_speed_1");
        optgroup->append_single_option_line("overhang_speed_2");
        optgroup->append_single_option_line("overhang_speed_3");

        optgroup = page->new_optgroup(L("Speed for non-print moves"));
        optgroup->append_single_option_line("travel_speed");
        optgroup->append_single_option_line("travel_speed_z");

        optgroup = page->new_optgroup(L("Modifiers"));
        optgroup->append_single_option_line("first_layer_speed");
        optgroup->append_single_option_line("first_layer_speed_over_raft");

        optgroup = page->new_optgroup(L("Acceleration control (advanced)"));
        optgroup->append_single_option_line("external_perimeter_acceleration");
        optgroup->append_single_option_line("perimeter_acceleration");
        optgroup->append_single_option_line("top_solid_infill_acceleration");
        optgroup->append_single_option_line("solid_infill_acceleration");
        optgroup->append_single_option_line("infill_acceleration");
        optgroup->append_single_option_line("bridge_acceleration");
        optgroup->append_single_option_line("first_layer_acceleration");
        optgroup->append_single_option_line("first_layer_acceleration_over_raft");
        optgroup->append_single_option_line("wipe_tower_acceleration");
        optgroup->append_single_option_line("travel_acceleration");
        optgroup->append_single_option_line("default_acceleration");

        optgroup = page->new_optgroup(L("Autospeed (advanced)"));
        optgroup->append_single_option_line("max_print_speed", "max-volumetric-speed_127176");
        optgroup->append_single_option_line("max_volumetric_speed", "max-volumetric-speed_127176");

        optgroup = page->new_optgroup(L("Pressure equalizer (experimental)"));
        optgroup->append_single_option_line("max_volumetric_extrusion_rate_slope_positive", "pressure-equlizer_331504");
        optgroup->append_single_option_line("max_volumetric_extrusion_rate_slope_negative", "pressure-equlizer_331504");

    page = add_options_page(L("Multiple Extruders"), "funnel");
        optgroup = page->new_optgroup(L("Extruders"));
        optgroup->append_single_option_line("perimeter_extruder");
        optgroup->append_single_option_line("infill_extruder");
        optgroup->append_single_option_line("solid_infill_extruder");
        optgroup->append_single_option_line("support_material_extruder");
        optgroup->append_single_option_line("support_material_interface_extruder");
        optgroup->append_single_option_line("wipe_tower_extruder");

        optgroup = page->new_optgroup(L("Ooze prevention"));
        optgroup->append_single_option_line("ooze_prevention");
        optgroup->append_single_option_line("standby_temperature_delta");

        optgroup = page->new_optgroup(L("Wipe tower"));
        optgroup->append_single_option_line("wipe_tower");
        optgroup->append_single_option_line("wipe_tower_width");
        optgroup->append_single_option_line("wipe_tower_brim_width");
        optgroup->append_single_option_line("wipe_tower_bridging");
        optgroup->append_single_option_line("wipe_tower_cone_angle");
        optgroup->append_single_option_line("wipe_tower_extra_spacing");
        optgroup->append_single_option_line("wipe_tower_extra_flow");
        optgroup->append_single_option_line("wipe_tower_no_sparse_layers");
        optgroup->append_single_option_line("single_extruder_multi_material_priming");

        optgroup = page->new_optgroup(L("Advanced"));
        optgroup->append_single_option_line("interface_shells");
        optgroup->append_single_option_line("mmu_segmented_region_max_width");
        optgroup->append_single_option_line("mmu_segmented_region_interlocking_depth");

    page = add_options_page(L("Advanced"), "wrench");
        optgroup = page->new_optgroup(L("Extrusion width"));
        optgroup->append_single_option_line("extrusion_width");
        optgroup->append_single_option_line("first_layer_extrusion_width");
        optgroup->append_single_option_line("perimeter_extrusion_width");
        optgroup->append_single_option_line("external_perimeter_extrusion_width");
        optgroup->append_single_option_line("infill_extrusion_width");
        optgroup->append_single_option_line("solid_infill_extrusion_width");
        optgroup->append_single_option_line("top_infill_extrusion_width");
        optgroup->append_single_option_line("support_material_extrusion_width");
        optgroup->append_single_option_line("automatic_extrusion_widths");

        optgroup = page->new_optgroup(L("Overlap"));
        optgroup->append_single_option_line("infill_overlap");

        optgroup = page->new_optgroup(L("Flow"));
        optgroup->append_single_option_line("bridge_flow_ratio");

        optgroup = page->new_optgroup(L("Slicing"));
        optgroup->append_single_option_line("slice_closing_radius");
        optgroup->append_single_option_line("slicing_mode");
        optgroup->append_single_option_line("resolution");
        optgroup->append_single_option_line("gcode_resolution");
        optgroup->append_single_option_line("arc_fitting");
        optgroup->append_single_option_line("xy_size_compensation");
        optgroup->append_single_option_line("elefant_foot_compensation", "elephant-foot-compensation_114487");

        optgroup = page->new_optgroup(L("Arachne perimeter generator"));
        optgroup->append_single_option_line("wall_transition_angle");
        optgroup->append_single_option_line("wall_transition_filter_deviation");
        optgroup->append_single_option_line("wall_transition_length");
        optgroup->append_single_option_line("wall_distribution_count");
        optgroup->append_single_option_line("min_bead_width");
        optgroup->append_single_option_line("min_feature_size");

    page = add_options_page(L("Output options"), "output+page_white");
        optgroup = page->new_optgroup(L("Sequential printing"));
        optgroup->append_single_option_line("complete_objects", "sequential-printing_124589");
        line = { L("Extruder clearance"), "" };
        line.append_option(optgroup->get_option("extruder_clearance_radius"));
        line.append_option(optgroup->get_option("extruder_clearance_height"));
        optgroup->append_line(line);

        optgroup = page->new_optgroup(L("Output file"));
        optgroup->append_single_option_line("gcode_comments");
        optgroup->append_single_option_line("gcode_label_objects");
        Option option = optgroup->get_option("output_filename_format");
        option.opt.full_width = true;
        optgroup->append_single_option_line(option);

        optgroup = page->new_optgroup(L("Other"));

        create_line_with_widget(optgroup.get(), "gcode_substitutions", "g-code-substitutions_301694", [this](wxWindow* parent) {
            return create_manage_substitution_widget(parent);
        });
        line = { "", "" };
        line.full_width = 1;
        line.widget = [this](wxWindow* parent) {
            return create_substitutions_widget(parent);
        };
        optgroup->append_line(line);

        optgroup = page->new_optgroup(L("Post-processing scripts"), 0);
        line = { "", "" };
        line.full_width = 1;
        line.widget = [this](wxWindow* parent) {
            return description_line_widget(parent, &m_post_process_explanation);
        };
        optgroup->append_line(line);
        option = optgroup->get_option("post_process");
        option.opt.full_width = true;
        option.opt.height = 5;//50;
        optgroup->append_single_option_line(option);

    page = add_options_page(L("Notes"), "note");
        optgroup = page->new_optgroup(L("Notes"), 0);
        option = optgroup->get_option("notes");
        option.opt.full_width = true;
        option.opt.height = 25;//250;
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

void EditorPrint::update_description_lines()
{
    AbstractEditor::update_description_lines();

    if (m_active_page && m_active_page->title() == "Layers and perimeters" && 
        m_recommended_thin_wall_thickness_description_line && m_top_bottom_shell_thickness_explanation)
    {
        const auto& ccc = m_preset_interactor.selected_config_container_context();
        m_recommended_thin_wall_thickness_description_line->SetText(
            WX::from_u8(Biz::Preset::PresetHints::recommended_thin_wall_thickness(ccc)));
        m_top_bottom_shell_thickness_explanation->SetText(
            WX::from_u8(Biz::Preset::PresetHints::top_bottom_shell_thickness_explanation(ccc)));
    }

    if (m_active_page && m_active_page->title() == "Output options") {
        if (m_post_process_explanation) {
            m_post_process_explanation->SetText(
                _L("Post processing scripts shall modify G-code file in place."));
            m_post_process_explanation->SetPathEnd("post-processing-scripts_283913");
        }
        // update G-code substitutions from the current configuration
        {
            m_subst_manager.update_from_config();
            if (m_del_all_substitutions_btn)
                m_del_all_substitutions_btn->Show(!m_subst_manager.is_empty_substitutions());
        }
    }
}

void EditorPrint::toggle_options()
{
    if (!m_active_page) return;

    m_config_manipulation.toggle_print_fff_options(*m_config_interactor);
}

void EditorPrint::update()
{
    m_update_cnt++;

    // see https://github.com/prusa3d/PrusaSlicer/issues/6814
    // ysFIXME: It's temporary workaround and should be clewer reworked:
    // Note: This workaround works till "support_material" and "overhangs" is exclusive sets of mutually no-exclusive parameters.
    // But it should be corrected when we will have more such sets.
    // Disable check of the compatibility of the "support_material" and "overhangs" options for saved user profile
    // NOTE: Initialization of the support_material_overhangs_queried value have to be processed just ones
    if (!m_config_manipulation.is_initialized_support_material_overhangs_queried())
    {
        const Slic3r::Preset& selected_preset = *m_config_interactor->preset_state().selected_preset;
        bool is_user_and_saved_preset = !selected_preset.is_system && !selected_preset.is_dirty;
        bool support_material_overhangs_queried = config().opt_bool("support_material") && !config().opt_bool("overhangs");
        m_config_manipulation.initialize_support_material_overhangs_queried(is_user_and_saved_preset && support_material_overhangs_queried);
    }

    m_config_manipulation.update_print_fff_config(*m_config_interactor, &config() ,true);

    update_description_lines();
    Layout();

    m_update_cnt--;

    if (m_update_cnt==0) {
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

void EditorPrint::clear_pages()
{
    AbstractEditor::clear_pages();

    m_recommended_thin_wall_thickness_description_line = nullptr;
    m_top_bottom_shell_thickness_explanation = nullptr;
    m_post_process_explanation = nullptr;

    m_del_all_substitutions_btn = nullptr;
}

// Return a callback to create a EditorPrint widget to edit G-code substitutions
wxSizer* EditorPrint::create_manage_substitution_widget(wxWindow* parent)
{
    auto create_btn = [parent](WX::ScalableButton** btn, const wxString& label, const std::string& icon_name) {
        *btn = new WX::ScalableButton(parent, wxID_ANY, icon_name, " " + label + " ", wxDefaultSize, wxDefaultPosition, wxBU_LEFT | wxBU_EXACTFIT);
        (*btn)->SetFont(WX::w_config()->normal_font());
        (*btn)->SetSize((*btn)->GetBestSize());
    };

    WX::ScalableButton* add_substitution_btn;
    create_btn(&add_substitution_btn, _L("Add"), "add_copies");
    add_substitution_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent e) {
        m_subst_manager.add_substitution();
        m_del_all_substitutions_btn->Show();
    });

    create_btn(&m_del_all_substitutions_btn, _L("Delete all"), "cross");
    m_del_all_substitutions_btn->Bind(wxEVT_BUTTON, [this, parent](wxCommandEvent e) {
        if (WX::MessageDialog(parent, _L("Are you sure you want to delete all substitutions?"), SLIC3R_APP_NAME, wxYES_NO | wxCANCEL | wxICON_QUESTION).
            ShowModal() != wxID_YES)
            return;
        m_subst_manager.delete_all();
        m_del_all_substitutions_btn->Hide();
    });

    auto sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(add_substitution_btn,        0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, WX::w_config()->em_unit(parent));
    sizer->Add(m_del_all_substitutions_btn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, WX::w_config()->em_unit(parent));

    parent->GetParent()->Layout();
    return sizer;
}

// Return a callback to create a EditorPrint widget to edit G-code substitutions
wxSizer* EditorPrint::create_substitutions_widget(wxWindow* parent)
{
    wxFlexGridSizer* grid_sizer = new wxFlexGridSizer(2, 5, WX::w_config()->em_unit()); // delete_button,  edit column contains "Find", "Replace", "Notes"
    grid_sizer->SetFlexibleDirection(wxBOTH);
    grid_sizer->AddGrowableCol(1);

    m_subst_manager.init(m_config_interactor.get(), parent, grid_sizer);
    m_subst_manager.set_cb_edited_substitution([this]() {
        update_dirty();
        Layout();
//!        wxGetApp().mainframe->on_config_changed(m_config); // invalidate print
    });
    m_subst_manager.set_cb_hide_delete_all_btn([this]() {
        m_del_all_substitutions_btn->Hide();
    });

    parent->GetParent()->Layout();
    return grid_sizer;
}

} 
