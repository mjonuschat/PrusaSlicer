#include "Slic3r/Biz/Config/ConfigLegacy.hpp"
#include "Slic3r/Domain/Config.hpp"

#include "Legacy/PrintConfig.hpp"

namespace Slic3r::Biz {

static std::vector<std::string> legacy_config_keys()
{
    static const std::vector<std::string> legacy_keys = {
        "arc_fitting", "autoemit_temperature_commands", "automatic_extrusion_widths", "automatic_infill_combination",
        "automatic_infill_combination_max_layer_height", "avoid_crossing_curled_overhangs", "avoid_crossing_perimeters",
        "avoid_crossing_perimeters_max_detour", "bed_custom_model", "bed_custom_texture", "bed_shape", "bed_temperature",
        "bed_temperature_extruder", "before_layer_gcode", "between_objects_gcode", "binary_gcode", "bottom_fill_pattern",
        "bottom_solid_layers", "bottom_solid_min_thickness", "bridge_acceleration", "bridge_angle", "bridge_fan_speed",
        "bridge_flow_ratio", "bridge_speed", "brim_separation", "brim_type", "brim_width", "chamber_minimal_temperature",
        "chamber_temperature", "color_change_gcode", "colorprint_heights", "compatible_printers_condition_cummulative",
        "complete_objects", "cooling", "cooling_tube_length", "cooling_tube_retraction", "default_acceleration",
        "default_filament_profile", "default_print_profile", "deretract_speed", "disable_fan_first_layers",
        "dont_support_bridges", "draft_shield", "duplicate_distance", "elefant_foot_compensation",
        "enable_dynamic_fan_speeds", "enable_dynamic_overhang_speeds", "end_filament_gcode", "end_gcode",
        "ensure_vertical_shell_thickness", "external_perimeter_acceleration", "external_perimeter_extrusion_width",
        "external_perimeter_speed", "external_perimeters_first", "extra_loading_move", "extra_perimeters",
        "extra_perimeters_on_overhangs", "extruder_clearance_height", "extruder_clearance_radius", "extruder_colour",
        "extruder_offset", "extrusion_axis", "extrusion_multiplier", "extrusion_width", "fan_always_on",
        "fan_below_layer_time", "filament_abrasive", "filament_colour", "filament_cooling_final_speed",
        "filament_cooling_initial_speed", "filament_cooling_moves", "filament_cost", "filament_density",
        "filament_deretract_speed", "filament_diameter", "filament_infill_max_crossing_speed", "filament_infill_max_speed",
        "filament_load_time", "filament_loading_speed", "filament_loading_speed_start", "filament_max_volumetric_speed",
        "filament_minimal_purge_on_wipe_tower", "filament_multitool_ramming", "filament_multitool_ramming_flow",
        "filament_multitool_ramming_volume", "filament_notes", "filament_purge_multiplier", "filament_ramming_parameters",
        "filament_retract_before_travel", "filament_retract_before_wipe", "filament_retract_layer_change",
        "filament_retract_length", "filament_retract_length_toolchange", "filament_retract_lift",
        "filament_retract_lift_above", "filament_retract_lift_below", "filament_retract_restart_extra",
        "filament_retract_restart_extra_toolchange", "filament_retract_speed", "filament_seam_gap_distance",
        "filament_settings_id", "filament_shrinkage_compensation_xy", "filament_shrinkage_compensation_z",
        "filament_soluble", "filament_spool_weight", "filament_stamping_distance", "filament_stamping_loading_speed",
        "filament_toolchange_delay", "filament_travel_lift_before_obstacle", "filament_travel_max_lift",
        "filament_travel_ramping_lift", "filament_travel_slope", "filament_type", "filament_unload_time",
        "filament_unloading_speed", "filament_unloading_speed_start", "filament_vendor", "filament_wipe", "fill_angle",
        "fill_density", "fill_pattern", "first_layer_acceleration", "first_layer_acceleration_over_raft",
        "first_layer_bed_temperature", "first_layer_extrusion_width", "first_layer_height", "first_layer_infill_speed",
        "first_layer_speed", "first_layer_speed_over_raft", "first_layer_temperature", "full_fan_speed_layer", "fuzzy_skin",
        "fuzzy_skin_point_dist", "fuzzy_skin_thickness", "gap_fill_enabled", "gap_fill_speed", "gcode_comments",
        "gcode_flavor", "gcode_label_objects", "gcode_resolution", "gcode_substitutions", "high_current_on_filament_swap",
        "host_type", "idle_temperature", "infill_acceleration", "infill_anchor", "infill_anchor_max", "infill_every_layers",
        "infill_extruder", "infill_extrusion_width", "infill_first", "infill_overlap", "infill_speed", "interface_shells",
        "interlocking_beam", "interlocking_beam_layer_count", "interlocking_beam_width", "interlocking_boundary_avoidance",
        "interlocking_depth", "interlocking_orientation", "ironing", "ironing_flowrate", "ironing_spacing", "ironing_speed",
        "ironing_type", "layer_gcode", "layer_height", "machine_limits_usage", "machine_max_acceleration_e",
        "machine_max_acceleration_extruding", "machine_max_acceleration_retracting", "machine_max_acceleration_travel",
        "machine_max_acceleration_x", "machine_max_acceleration_y", "machine_max_acceleration_z", "machine_max_feedrate_e",
        "machine_max_feedrate_x", "machine_max_feedrate_y", "machine_max_feedrate_z", "machine_max_jerk_e",
        "machine_max_jerk_x", "machine_max_jerk_y", "machine_max_jerk_z", "machine_min_extruding_rate",
        "machine_min_travel_rate", "max_fan_speed", "max_layer_height", "max_print_height", "max_print_speed",
        "max_volumetric_extrusion_rate_slope_negative", "max_volumetric_extrusion_rate_slope_positive",
        "max_volumetric_speed", "min_bead_width", "min_fan_speed", "min_feature_size", "min_layer_height", "min_print_speed",
        "min_skirt_length", "mmu_segmented_region_interlocking_depth", "mmu_segmented_region_max_width",
        "multimaterial_purging", "notes", "nozzle_diameter", "nozzle_high_flow", "only_one_perimeter_first_layer",
        "only_retract_when_crossing_perimeters", "ooze_prevention", "output_filename_format", "over_bridge_speed",
        "overhang_fan_speed_0", "overhang_fan_speed_1", "overhang_fan_speed_2", "overhang_fan_speed_3", "overhang_speed_0",
        "overhang_speed_1", "overhang_speed_2", "overhang_speed_3", "overhangs", "parking_pos_retraction",
        "pause_print_gcode", "perimeter_acceleration", "perimeter_extruder", "perimeter_extrusion_width",
        "perimeter_generator", "perimeter_speed", "perimeters", "physical_printer_settings_id", "post_process",
        "prefer_clockwise_movements", "print_settings_id", "printer_model", "printer_notes", "printer_settings_id",
        "printer_technology", "printer_variant", "printer_vendor", "raft_contact_distance", "raft_expansion",
        "raft_first_layer_density", "raft_first_layer_expansion", "raft_layers", "remaining_times", "resolution",
        "retract_before_travel", "retract_before_wipe", "retract_layer_change", "retract_length",
        "retract_length_toolchange", "retract_lift", "retract_lift_above", "retract_lift_below", "retract_restart_extra",
        "retract_restart_extra_toolchange", "retract_speed", "scarf_seam_entire_loop", "scarf_seam_length",
        "scarf_seam_max_segment_length", "scarf_seam_on_inner_perimeters", "scarf_seam_only_on_smooth",
        "scarf_seam_placement", "scarf_seam_start_height", "seam_gap_distance", "seam_position", "silent_mode",
        "single_extruder_multi_material", "single_extruder_multi_material_priming", "skirt_distance", "skirt_height",
        "skirts", "slice_closing_radius", "slicing_mode", "slowdown_below_layer_time", "small_perimeter_speed",
        "solid_infill_acceleration", "solid_infill_below_area", "solid_infill_every_layers", "solid_infill_extruder",
        "solid_infill_extrusion_width", "solid_infill_speed", "spiral_vase", "staggered_inner_seams",
        "standby_temperature_delta", "start_filament_gcode", "start_gcode", "support_material", "support_material_angle",
        "support_material_auto", "support_material_bottom_contact_distance", "support_material_bottom_interface_layers",
        "support_material_buildplate_only", "support_material_closing_radius", "support_material_contact_distance",
        "support_material_enforce_layers", "support_material_extruder", "support_material_extrusion_width",
        "support_material_interface_contact_loops", "support_material_interface_extruder",
        "support_material_interface_layers", "support_material_interface_pattern", "support_material_interface_spacing",
        "support_material_interface_speed", "support_material_pattern", "support_material_spacing", "support_material_speed",
        "support_material_style", "support_material_synchronize_layers", "support_material_threshold",
        "support_material_with_sheath", "support_material_xy_spacing", "support_tree_angle", "support_tree_angle_slow",
        "support_tree_branch_diameter", "support_tree_branch_diameter_angle", "support_tree_branch_diameter_double_wall",
        "support_tree_branch_distance", "support_tree_tip_diameter", "support_tree_top_rate", "temperature",
        "template_custom_gcode", "thick_bridges", "thin_walls", "thumbnails", "thumbnails_format", "toolchange_gcode",
        "top_fill_pattern", "top_infill_extrusion_width", "top_one_perimeter_type", "top_solid_infill_acceleration",
        "top_solid_infill_speed", "top_solid_layers", "top_solid_min_thickness", "travel_acceleration",
        "travel_lift_before_obstacle", "travel_max_lift", "travel_ramping_lift", "travel_slope", "travel_speed",
        "travel_speed_z", "use_firmware_retraction", "use_relative_e_distances", "use_volumetric_e",
        "variable_layer_height", "wall_distribution_count", "wall_transition_angle", "wall_transition_filter_deviation",
        "wall_transition_length", "wipe", "wipe_into_infill", "wipe_into_objects", "wipe_tower", "wipe_tower_acceleration",
        "wipe_tower_bridging", "wipe_tower_brim_width", "wipe_tower_cone_angle", "wipe_tower_extra_flow",
        "wipe_tower_extra_spacing", "wipe_tower_extruder", "wipe_tower_no_sparse_layers", "wipe_tower_width",
        "wiping_volumes_matrix", "wiping_volumes_use_custom_matrix", "xy_size_compensation", "z_offset"
    };
    return legacy_keys;
}



static std::vector<std::string> legacy_filament_overrides_keys()
{
    std::vector<std::string> out = {
        // floats
        "retract_length", "retract_lift", "retract_lift_above", "retract_lift_below", "retract_speed",
        "travel_max_lift", "deretract_speed", "retract_restart_extra", "retract_before_travel",
        "retract_length_toolchange", "retract_restart_extra_toolchange",
        // bools
        "retract_layer_change", "wipe", "travel_lift_before_obstacle", "travel_ramping_lift",
        // percents
        "retract_before_wipe", "travel_slope",
        // floatsorpercents
        "seam_gap_distance" };
    for (std::string& override_key : out)
        override_key = std::string("filament_") + override_key;
    return out;
}



static void convert_enum(const Slic3rLegacy::ConfigOption* co, Domain::ConfigItem& item)
{
    const std::string old_str = co->serialize();
    for (const Domain::EnumValueDef& evd : item.def().enum_values)
        if (evd.str_serialized == old_str)
            item.set_enum_from_string(old_str);
}


static Slic3rLegacy::DynamicPrintConfig load_legacy_config_from_legacy_file(const std::string& filename)
{
    using namespace Slic3rLegacy;
    DynamicPrintConfig cfg;
    ForwardCompatibilitySubstitutionRule substitutions_ctxt = ForwardCompatibilitySubstitutionRule::EnableSilent;
    cfg.load(filename, substitutions_ctxt);
    return cfg;
}



static bool convert_old_to_new(const Slic3rLegacy::ConfigOption* opt, Domain::ConfigItem& item, int filament_id = -1)
{
    using namespace Slic3rLegacy;

    if (opt->type() == coBool && item.type() == Domain::ConfigItemType::Bool)
        item.set(opt->getBool());
    else if (opt->type() == coInt && item.type() == Domain::ConfigItemType::Int)
        item.set(opt->getInt());
    else if (opt->type() == coFloat && item.type() == Domain::ConfigItemType::Double)
        item.set(opt->getFloat());
    else if (opt->type() == coString && item.type() == Domain::ConfigItemType::String)
        item.set(static_cast<const ConfigOptionString*>(opt)->value);
    else if (opt->type() == coEnum && item.type() == Domain::ConfigItemType::Enum)
        convert_enum(opt, item);
    else if (opt->type() == coFloatOrPercent && item.type() == Domain::ConfigItemType::FloatOrPercent) {
        bool is_percent = static_cast<const ConfigOptionFloatOrPercent*>(opt)->percent;
        double value = static_cast<const ConfigOptionFloatOrPercent*>(opt)->value;
        Domain::FloatOrPercentage fop = is_percent ? Domain::Percentage(value) : Domain::FloatOrPercentage(value);
        item.set(fop);
    }            
    else if (opt->type() == coPercent && item.type() == Domain::ConfigItemType::Percent)
        item.set(Domain::Percentage(static_cast<const ConfigOptionPercent*>(opt)->value));
    else if (opt->type() == coBools && item.type() == Domain::ConfigItemType::Bools) {
        std::vector<unsigned char> old_vec = static_cast<const ConfigOptionBools*>(opt)->values;
        std::vector<bool> vec(old_vec.begin(), old_vec.end());
        item.set(vec);
    }
    else if (opt->type() == coInts && item.type() == Domain::ConfigItemType::Ints)
        item.set(static_cast<const ConfigOptionInts*>(opt)->values);
    else if (opt->type() == coFloats && item.type() == Domain::ConfigItemType::Doubles)
        item.set(static_cast<const ConfigOptionFloats*>(opt)->values);
    else if (opt->type() == coStrings && item.type() == Domain::ConfigItemType::Strings)
        item.set(static_cast<const ConfigOptionStrings*>(opt)->values);
    else if (opt->type() == coPoints && item.type() == Domain::ConfigItemType::Points) {
        const std::vector<Slic3rLegacy::Vec2d> old_vec = static_cast<const ConfigOptionPoints*>(opt)->values;
        std::vector<Domain::Vec2d> vec(old_vec.begin(), old_vec.end());
        item.set(vec);
    }
    else if (opt->is_vector() && filament_id != -1) {
        // This vector actually contains scalar values to be assigned to
        // different print / toolprint settings.
        if (opt->type() == coBools && item.type() == Domain::ConfigItemType::Bool)
            item.set(static_cast<const ConfigOptionBools*>(opt)->get_at(filament_id));
        else if (opt->type() == coInts && item.type() == Domain::ConfigItemType::Int)
            item.set(static_cast<const ConfigOptionInts*>(opt)->get_at(filament_id));
        else if (opt->type() == coInts && item.type() == Domain::ConfigItemType::IntOptional) {
            if (static_cast<const ConfigOptionInts*>(opt)->is_nil(filament_id))
                item.set(std::optional<int>());
            else
                item.set(std::optional<int>(static_cast<const ConfigOptionInts*>(opt)->get_at(filament_id)));
        }
        else if (opt->type() == coFloats && item.type() == Domain::ConfigItemType::Double)
            item.set(static_cast<const ConfigOptionFloats*>(opt)->get_at(filament_id));
        else if (opt->type() == coStrings && item.type() == Domain::ConfigItemType::String)
            item.set(static_cast<const ConfigOptionStrings*>(opt)->get_at(filament_id));
        else if (opt->type() == coPercents && item.type() == Domain::ConfigItemType::Percent)
            item.set(Domain::Percentage(static_cast<const ConfigOptionPercents*>(opt)->get_at(filament_id)));
        else if (opt->type() == coFloatsOrPercents && item.type() == Domain::ConfigItemType::FloatOrPercent) {
            bool is_percent = static_cast<const ConfigOptionFloatsOrPercents*>(opt)->get_at(filament_id).percent;
            double value = static_cast<const ConfigOptionFloatsOrPercents*>(opt)->get_at(filament_id).value;
            Domain::FloatOrPercentage fop = is_percent ? Domain::Percentage(value) : Domain::FloatOrPercentage(value);
            item.set(fop);
        }
        else if (opt->type() == coPoints && item.type() == Domain::ConfigItemType::Point) {
            item.set(Domain::Vec2d(static_cast<const ConfigOptionPoints*>(opt)->values[filament_id]));
        }
        else {
            // Old and new types do not match.
            return false;
        }
    }
    else {
        // Old and new types do not match.
        return false;
    }
    return true;
}



static bool convert_new_to_old(const Domain::ConfigItem& item, Slic3rLegacy::ConfigOption* opt, const Slic3rLegacy::ConfigOptionDef& def_old, int filament_id = -1)
{
    using namespace Slic3rLegacy;

    if (opt->type() == coBool && item.type() == Domain::ConfigItemType::Bool)
        static_cast<ConfigOptionBool*>(opt)->value = item.get<bool>();
    else if (opt->type() == coInt && item.type() == Domain::ConfigItemType::Int)
        static_cast<ConfigOptionInt*>(opt)->value = item.get<int>();
    else if (opt->type() == coFloat && item.type() == Domain::ConfigItemType::Double)
        static_cast<ConfigOptionFloat*>(opt)->value = item.get<double>();
    else if (opt->type() == coString && item.type() == Domain::ConfigItemType::String)
        static_cast<ConfigOptionString*>(opt)->value = item.get<std::string>();
    else if (opt->type() == coEnum && item.type() == Domain::ConfigItemType::Enum) {
        std::string new_ser = std::string(item.get_enum_strings().first);
        if (def_old.has_enum_value(new_ser))
            opt->deserialize(new_ser);
        else {
            // Old enum does not have this value.
            return false;
        }
    }
    else if (opt->type() == coFloatOrPercent && item.type() == Domain::ConfigItemType::FloatOrPercent) {
        bool is_percent = item.get<Domain::FloatOrPercentage>().is_percentage();
        static_cast<ConfigOptionFloatOrPercent*>(opt)->percent = is_percent;
        static_cast<ConfigOptionFloatOrPercent*>(opt)->value = is_percent
            ? item.get<Domain::FloatOrPercentage>().percentage().value
            : item.get<Domain::FloatOrPercentage>().float_value();
    }            
    else if (opt->type() == coPercent && item.type() == Domain::ConfigItemType::Percent)
        static_cast<ConfigOptionPercent*>(opt)->value = item.get<Domain::Percentage>().value;
    else if (opt->type() == coBools && item.type() == Domain::ConfigItemType::Bools) {
        std::vector<bool> new_vec = item.get<std::vector<bool>>();
        std::vector<unsigned char> old_vec(new_vec.begin(), new_vec.end());
        static_cast<ConfigOptionBools*>(opt)->values = old_vec;
    }
    else if (opt->type() == coInts && item.type() == Domain::ConfigItemType::Ints)
        static_cast<ConfigOptionInts*>(opt)->values = item.get<std::vector<int>>();
    else if (opt->type() == coFloats && item.type() == Domain::ConfigItemType::Doubles)
        static_cast<ConfigOptionFloats*>(opt)->values = item.get<std::vector<double>>();
    else if (opt->type() == coStrings && item.type() == Domain::ConfigItemType::Strings)
        static_cast<ConfigOptionStrings*>(opt)->values = item.get<std::vector<std::string>>();
    else if (opt->type() == coPoints && item.type() == Domain::ConfigItemType::Points) {
        const auto& new_vec = item.get<std::vector<Domain::Vec2d>>();
        std::vector<Slic3rLegacy::Vec2d> old_vec(new_vec.begin(), new_vec.end());
        static_cast<ConfigOptionPoints*>(opt)->values = old_vec;
    }
    else if (opt->is_vector() && filament_id != -1) {
        if (opt->type() == coBools && item.type() == Domain::ConfigItemType::Bool) {
            static_cast<ConfigOptionBools*>(opt)->values.resize(filament_id + 1);
            static_cast<ConfigOptionBools*>(opt)->values[filament_id] = item.is_null()
                ? static_cast<ConfigOptionBools*>(opt)->nil_value()
                : item.get<bool>();
        }
        else if (opt->type() == coInts && item.type() == Domain::ConfigItemType::Int) {
            static_cast<ConfigOptionInts*>(opt)->values.resize(filament_id + 1);
            static_cast<ConfigOptionInts*>(opt)->values[filament_id] = item.get<int>();
        }
        else if (opt->type() == coInts && item.type() == Domain::ConfigItemType::IntOptional) {
            static_cast<ConfigOptionInts*>(opt)->values.resize(filament_id + 1);
            const auto& idle = item.get<std::optional<int>>();
            static_cast<ConfigOptionInts*>(opt)->values[filament_id] = idle
                ? *idle
                : static_cast<ConfigOptionInts*>(opt)->nil_value();
        }
        else if (opt->type() == coFloats && item.type() == Domain::ConfigItemType::Double) {
            static_cast<ConfigOptionFloats*>(opt)->values.resize(filament_id + 1);
            static_cast<ConfigOptionFloats*>(opt)->values[filament_id] = item.is_null()
                ? static_cast<ConfigOptionFloats*>(opt)->nil_value()
                : item.get<double>();
        }
        else if (opt->type() == coStrings && item.type() == Domain::ConfigItemType::String) {
            static_cast<ConfigOptionStrings*>(opt)->values.resize(filament_id + 1);
            static_cast<ConfigOptionStrings*>(opt)->values[filament_id] = item.get<std::string>();
        }
        else if (opt->type() == coPercents && item.type() == Domain::ConfigItemType::Percent) {
            static_cast<ConfigOptionPercents*>(opt)->values.resize(filament_id + 1);
            static_cast<ConfigOptionPercents*>(opt)->values[filament_id] = item.is_null()
                ? static_cast<ConfigOptionPercents*>(opt)->nil_value()
                : item.get<Domain::Percentage>().value;
        }
        else if (opt->type() == coFloatsOrPercents && item.type() == Domain::ConfigItemType::FloatOrPercent) {
            static_cast<ConfigOptionFloatsOrPercents*>(opt)->values.resize(filament_id + 1);
            if (item.is_null()) {
                static_cast<ConfigOptionFloatsOrPercents*>(opt)->values[filament_id] = static_cast<ConfigOptionFloatsOrPercents*>(opt)->nil_value();
            }
            else {
                Slic3rLegacy::FloatOrPercent fop;
                fop.percent = item.get<Domain::FloatOrPercentage>().is_percentage();
                fop.value = fop.percent
                    ? item.get<Domain::FloatOrPercentage>().percentage().value
                    : item.get<Domain::FloatOrPercentage>().float_value();
                static_cast<ConfigOptionFloatsOrPercents*>(opt)->values[filament_id] = fop;
            }
        }
        else if (opt->type() == coPoints && item.type() == Domain::ConfigItemType::Point) {
            static_cast<ConfigOptionPoints*>(opt)->values.resize(filament_id + 1);
            static_cast<ConfigOptionPoints*>(opt)->values[filament_id] = Slic3rLegacy::Vec2d(item.get<Domain::Vec2d>());
        }
        else {
            // Old and new types do not match.
            return false;
        }
    }
    else {
        // Old and new types do not match.
        return false;
    }
    return true;
}



static void fill_config_box_from_legacy(const Slic3rLegacy::DynamicPrintConfig& cfg,
                                        Domain::ConfigBox& box,
                                        int filament_id = -1)
{
    using namespace Slic3rLegacy;

    // These old config options had filament override in PrusaSlicer 2.9.2. The overrides were
    // distinguished from the actual values by prepending "filament_". Now, both the values and the override
    // have the same key, but they live in different boxes.
    const std::vector<std::string> filament_overrides_keys = legacy_filament_overrides_keys();

    for (const std::string& old_key : cfg.keys()) {
        bool has_override = std::ranges::find(filament_overrides_keys, std::string("filament_") + old_key) != filament_overrides_keys.end();
        bool is_filament_override = std::ranges::find(filament_overrides_keys, old_key) != filament_overrides_keys.end();
        ASSERT(! is_filament_override || boost::starts_with(old_key, "filament_"));
        std::string new_key(old_key.begin() + (is_filament_override ? 9 : 0), old_key.end()); // 9 = trim "filament_"

        if (! box.contains(new_key))
            continue;

        const ConfigOption* opt = cfg.option(old_key);
        Domain::ConfigItem& item = box.opt(new_key);

        if (is_filament_override) {
            if (filament_id == -1
                || std::ranges::find(item.def().overrides_in, "filament_settings") == item.def().overrides_in.end()
                || box.type() != "filament_settings"
                || !opt->is_vector()
                || static_cast<const Slic3rLegacy::ConfigOptionVectorBase*>(opt)->is_nil(filament_id))
                continue;
        }

        if (has_override && box.type() == "filament_settings") {
            // This config option has override in filament settings. Only continue
            // if current box is NOT filament settings, otherwise we may overwrite
            // what we already loaded as an override.
            continue;
        }

        if (filament_id != -1 && ! opt->is_vector()) {
            // This box exists once per filament, but the old value is not vector.
            continue;
        }
        if (convert_old_to_new(opt, item, filament_id)) {
            if (is_filament_override && ! opt->is_nil())
                item.set_null(false);
        }
    }
}


static FDMLegacyConfigPack convert_legacy_fdm_config(Slic3rLegacy::DynamicPrintConfig& cfg)
{
    using namespace Slic3rLegacy;
    int extruder_num = cfg.has("nozzle_diameter") ? int(cfg.option<ConfigOptionFloats>("nozzle_diameter")->size()) : 1;
    FDMLegacyConfigPack out;
    out.toolprint_settings.resize(extruder_num);
    out.filament_settings.resize(extruder_num);
    fill_config_box_from_legacy(cfg, out.printer_settings);
    fill_config_box_from_legacy(cfg, out.print_settings);
    for (int i = 0; i < extruder_num; ++i) {
        fill_config_box_from_legacy(cfg, out.toolprint_settings[i], i);
        fill_config_box_from_legacy(cfg, out.filament_settings[i], i);
    }
    return out;
}



static SLALegacyConfigPack convert_legacy_sla_config(Slic3rLegacy::DynamicPrintConfig& cfg)
{
    SLALegacyConfigPack out;

    // TODO

    return out;
}



std::variant<FDMLegacyConfigPack, SLALegacyConfigPack> load_config_from_legacy_file(const std::string& filename)
{
    using namespace Slic3rLegacy;
    DynamicPrintConfig cfg = load_legacy_config_from_legacy_file(filename);

    if (cfg.has("printer_technology")) {
        if (auto pt = cfg.opt_enum<PrinterTechnology>("printer_technology"); pt == ptFFF)
            return convert_legacy_fdm_config(cfg);
        else if (pt == ptSLA)
            return convert_legacy_sla_config(cfg);
    }
    throw std::runtime_error("Loading legacy config failed: missing or incorrect printer technology.");
}



std::string serialize_as_legacy_config(const FDMLegacyConfigPack& cfg)
{
    ASSERT(cfg.filament_settings.size() == cfg.toolprint_settings.size());
    using namespace Slic3rLegacy;

    // Configs were exported flat in PrusaSlicer <= 2.9.2. This is the list of keys we should try to fill in.
    const std::vector<std::string> legacy_keys = legacy_config_keys();
    const std::vector<std::string> filament_overrides_keys = legacy_filament_overrides_keys();

    std::vector<std::pair<const Domain::ConfigBox*, int>> boxes = { {&cfg.printer_settings, -1}, {&cfg.print_settings, -1} };
    for (int i = 0; i<cfg.toolprint_settings.size(); ++i)
        boxes.emplace_back(&cfg.toolprint_settings[i], i);
    for (int i = 0; i<cfg.filament_settings.size(); ++i)
        boxes.emplace_back(&cfg.filament_settings[i], i);
    


    std::unique_ptr<DynamicPrintConfig> cfg_old(DynamicPrintConfig::new_from_defaults_keys(legacy_keys));
    for (const std::string& key : cfg_old->keys()) {
        ConfigOption* opt = cfg_old->option(key);

        for (const auto& [box, filament_id] : boxes) {
            std::optional<const Domain::ConfigItem*> item = box->contains(key);
            if (item.has_value() && ! (*item)->is_nullable()) {
                convert_new_to_old(**item, opt, *cfg_old->def()->get(key), filament_id);
            }
            if (!item.has_value() && box->type() == "filament_settings"
             && std::ranges::find(filament_overrides_keys, key) != filament_overrides_keys.end()) {
                // This old item is not present in any of the boxes, but it is an override of something.
                // PrusaSlicer 2.9.2 had all overrides at filament level. Find the current override.
                ASSERT(boost::starts_with(key, "filament_"));
                std::string new_key(key.begin() + 9, key.end());
                const Domain::ConfigItem& over = box->opt(new_key);
                convert_new_to_old(over, opt, *cfg_old->def()->get(new_key), filament_id);
            }
        }
    }

    std::string out;
    for (const std::string& key : cfg_old->keys()) {
        out += "; " + key + " = " + cfg_old->opt_serialize(key) + "\n";
    }
    return out;
}
    


// TODO: New slicer changed enums PrintHostType and AuthorizationType (=PrintHostAuthType).
// We need to convert old options to the new ones properly. BEWARE especially of PrusaConnect and
// PrusaConnectNew. PrusaConnect was removed and PrusaConnectNew was renamed to PrusaConnect.

} // namespace Slic3r::Biz
