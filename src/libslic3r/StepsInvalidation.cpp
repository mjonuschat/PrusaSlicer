#include "StepsInvalidation.hpp"

#include <variant>

#include "libslic3r/Config.hpp"
#include "libslic3r/Print.hpp"

namespace Slic3r::SlicingSync {

using Domain::ConfigView;
using Domain::Percentage;

template <typename Set>
AllOrSome<Set> merge (const AllOrSome<Set>& a, const AllOrSome<Set>& b) {
    if (std::holds_alternative<AllSteps>(a) || std::holds_alternative<AllSteps>(b)) {
        return AllSteps{};
    }

    Set values_a{std::get<Set>(a)};
    Set values_b{std::get<Set>(b)};
    values_a.merge(values_b);
    return values_a;
}
template AllOrSome<PrintSteps> merge(const AllOrSome<PrintSteps>& a, const AllOrSome<PrintSteps>& b);
template AllOrSome<PrintObjectSteps> merge(const AllOrSome<PrintObjectSteps>& a, const AllOrSome<PrintObjectSteps>& b);

StepsPerPrintObject merge(const StepsPerPrintObject& a, const StepsPerPrintObject& b) {
    StepsPerPrintObject result{a};
    StepsPerPrintObject same_key_elements{b};

    result.merge(same_key_elements);

    for (const auto& [print_object, invalidated_steps] : same_key_elements) {
        result.at(print_object) = merge(result.at(print_object), invalidated_steps);
    }

    return result;
}

InvalidatedSteps merge(const InvalidatedSteps& a, const InvalidatedSteps& b) {
    return {merge(a.print, b.print), merge(a.object, b.object)};
}


InvalidatedSteps merge(const std::vector<InvalidatedSteps>& invalidated_steps) {
    InvalidatedSteps result;
    for (const InvalidatedSteps& steps : invalidated_steps) {
        result = merge(result, steps);
    }
    return result;
}

std::pair<AllOrSome<PrintSteps>, AllOrSome<PrintObjectSteps>> diff_to_invalidated_steps(
    const ConfigView& old_config,
    const ConfigView& new_config,
    const std::vector<std::string>& diff
)
{
    if (diff.empty()) {
        return {PrintSteps{}, PrintObjectSteps{}};
    }

    PrintSteps print_steps{};
    PrintObjectSteps object_steps{};
    for (const std::string &opt_key : diff) {
        if (   opt_key == "brim_width"
            || opt_key == "brim_separation"
            || opt_key == "brim_type") {
            object_steps.insert(posSupportSpotsSearch);
            // Brim is printed below supports, support invalidates brim and skirt.
            object_steps.insert(posSupportMaterial);
        } else if (
               opt_key == "perimeters"
            || opt_key == "extra_perimeters"
            || opt_key == "extra_perimeters_on_overhangs"
            || opt_key == "first_layer_extrusion_width"
            || opt_key == "perimeter_extrusion_width"
            || opt_key == "infill_overlap"
            || opt_key == "external_perimeters_first"
            || opt_key == "arc_fitting"
            || opt_key == "top_one_perimeter_type"
            || opt_key == "only_one_perimeter_first_layer") {
            object_steps.insert(posPerimeters);
        } else if (
               opt_key == "gap_fill_enabled"
            || opt_key == "gap_fill_speed") {
            // Return true if gap-fill speed has changed from zero value to non-zero or from non-zero value to zero.
            auto is_gap_fill_changed_state_due_to_speed = [&opt_key, &old_config, &new_config]() -> bool {
                if (opt_key == "gap_fill_speed") {
                    const auto old_gap_fill_speed = old_config.get<double>(opt_key);
                    const auto new_gap_fill_speed = new_config.get<double>(opt_key);
                    assert(old_gap_fill_speed && new_gap_fill_speed);
                    return (old_gap_fill_speed > 0.f && new_gap_fill_speed == 0.f) ||
                           (old_gap_fill_speed == 0.f && new_gap_fill_speed > 0.f);
                }
                return false;
            };

            // Filtering of unprintable regions in multi-material segmentation depends on if gap-fill is enabled or not.
            // So step posSlice is invalidated when gap-fill was enabled/disabled by option "gap_fill_enabled" or by
            // changing "gap_fill_speed" to force recomputation of the multi-material segmentation.

            // TODO
            //if (this->is_mm_painted() && (opt_key == "gap_fill_enabled" || (opt_key == "gap_fill_speed" && is_gap_fill_changed_state_due_to_speed())))
            //    result.object_steps.insert(posSlice);

            object_steps.insert(posPerimeters);
        } else if (
               opt_key == "layer_height"
            || opt_key == "mmu_segmented_region_max_width"
            || opt_key == "mmu_segmented_region_interlocking_depth"
            || opt_key == "raft_layers"
            || opt_key == "raft_contact_distance"
            || opt_key == "slice_closing_radius"
            || opt_key == "slicing_mode"
            || opt_key == "interlocking_beam"
            || opt_key == "interlocking_orientation"
            || opt_key == "interlocking_beam_layer_count"
            || opt_key == "interlocking_depth"
            || opt_key == "interlocking_boundary_avoidance"
            || opt_key == "interlocking_beam_width") {
            object_steps.insert(posSlice);
		} else if (
               opt_key == "elefant_foot_compensation"
            || opt_key == "support_material_contact_distance" 
            || opt_key == "xy_size_compensation") {
            object_steps.insert(posSlice);
        } else if (opt_key == "support_material") {
            object_steps.insert(posSupportMaterial);

            // TODO
            /*
            if (m_config.support_material_contact_distance == 0.) {
            	// Enabling / disabling supports while soluble support interface is enabled.
            	// This changes the bridging logic (bridging enabled without supports, disabled with supports).
            	// Reset everything.
            	// See GH #1482 for details.
	            result.object_steps.insert(posSlice);
	        }
            */
        } else if (
        	   opt_key == "support_material_auto"
            || opt_key == "support_material_angle"
            || opt_key == "support_material_buildplate_only"
            || opt_key == "support_material_enforce_layers"
            || opt_key == "support_material_extruder"
            || opt_key == "support_material_extrusion_width"
            || opt_key == "support_material_bottom_contact_distance"
            || opt_key == "support_material_interface_layers"
            || opt_key == "support_material_bottom_interface_layers"
            || opt_key == "support_material_interface_pattern"
            || opt_key == "support_material_interface_contact_loops"
            || opt_key == "support_material_interface_extruder"
            || opt_key == "support_material_interface_spacing"
            || opt_key == "support_material_pattern"
            || opt_key == "support_material_style"
            || opt_key == "support_material_xy_spacing"
            || opt_key == "support_material_spacing"
            || opt_key == "support_material_closing_radius"
            || opt_key == "support_material_synchronize_layers"
            || opt_key == "support_material_threshold"
            || opt_key == "support_material_with_sheath"
            || opt_key == "support_tree_angle"
            || opt_key == "support_tree_angle_slow"
            || opt_key == "support_tree_branch_diameter"
            || opt_key == "support_tree_branch_diameter_angle"
            || opt_key == "support_tree_branch_diameter_double_wall"
            || opt_key == "support_tree_top_rate"
            || opt_key == "support_tree_branch_distance"
            || opt_key == "support_tree_tip_diameter"
            || opt_key == "raft_expansion"
            || opt_key == "raft_first_layer_density"
            || opt_key == "raft_first_layer_expansion"
            || opt_key == "dont_support_bridges"
            || opt_key == "first_layer_extrusion_width") {
            object_steps.insert(posSupportMaterial);
        } else if (opt_key == "bottom_solid_layers") {
            object_steps.insert(posPrepareInfill);

            //TODO
            /*
            if (m_print->config().spiral_vase) {
                // Changing the number of bottom layers when a spiral vase is enabled requires re-slicing the object again.
                // Otherwise, holes in the bottom layers could be filled, as is reported in GH #5528.
                result.object_steps.insert(posSlice);
            }
            */
        } else if (
               opt_key == "interface_shells"
            || opt_key == "infill_only_where_needed"
            || opt_key == "infill_every_layers"
            || opt_key == "automatic_infill_combination"
            || opt_key == "automatic_infill_combination_max_layer_height"
            || opt_key == "solid_infill_every_layers"
            || opt_key == "ensure_vertical_shell_thickness"
            || opt_key == "bottom_solid_min_thickness"
            || opt_key == "top_solid_layers"
            || opt_key == "top_solid_min_thickness"
            || opt_key == "solid_infill_below_area"
            || opt_key == "infill_extruder"
            || opt_key == "solid_infill_extruder"
            || opt_key == "infill_extrusion_width"
            || opt_key == "bridge_angle") {
            object_steps.insert(posPrepareInfill);
        } else if (
               opt_key == "top_fill_pattern"
            || opt_key == "bottom_fill_pattern"
            || opt_key == "external_fill_link_max_length"
            || opt_key == "fill_angle"
            || opt_key == "infill_anchor"
            || opt_key == "infill_anchor_max"
            || opt_key == "top_infill_extrusion_width"
            || opt_key == "first_layer_extrusion_width") {
            object_steps.insert(posInfill);
        } else if (opt_key == "fill_pattern") {
            object_steps.insert(posPrepareInfill);
        } else if (opt_key == "over_bridge_speed") {
            const auto old_speed = old_config.get<double>(opt_key);
            const auto new_speed = new_config.get<double>(opt_key);
            if ( old_speed == 0 || new_speed == 0) {
                object_steps.insert(posPrepareInfill);
            }
            print_steps.insert(psGCodeExport);
        } else if (opt_key == "fill_density") {
            // One likely wants to reslice only when switching between zero infill to simulate boolean difference (subtracting volumes),
            // normal infill and 100% (solid) infill.
            const auto old_density = old_config.get<Percentage>(opt_key);
            const auto new_density = new_config.get<Percentage>(opt_key);
            //FIXME Vojtech is not quite sure about the 100% here, maybe it is not needed.
            if (is_approx(old_density.value, 0.) || is_approx(old_density.value, 100.) ||
                is_approx(new_density.value, 0.) || is_approx(new_density.value, 100.))
                object_steps.insert(posPerimeters);
            object_steps.insert(posPrepareInfill);
        } else if (opt_key == "solid_infill_extrusion_width") {
            // This value is used for calculating perimeter - infill overlap, thus perimeters need to be recalculated.
            object_steps.insert(posPerimeters);
            object_steps.insert(posPrepareInfill);
        } else if (
               opt_key == "external_perimeter_extrusion_width"
            || opt_key == "perimeter_extruder"
            || opt_key == "fuzzy_skin"
            || opt_key == "fuzzy_skin_thickness"
            || opt_key == "fuzzy_skin_point_dist"
            || opt_key == "overhangs"
            || opt_key == "thin_walls"
            || opt_key == "thick_bridges") {
            object_steps.insert(posPerimeters);
            object_steps.insert(posSupportMaterial);
        } else if (opt_key == "bridge_flow_ratio") {

            // TODO
            /*
            if (m_config.support_material_contact_distance > 0.) {
            	// Only invalidate due to bridging if bridging is enabled.
            	// If later "support_material_contact_distance" is modified, the complete PrintObject is invalidated anyway.
            	result.object_steps.insert(posPerimeters);
            	result.object_steps.insert(posInfill);
	            result.object_steps.insert(posSupportMaterial);
	        }
            */
        } else if (
            opt_key == "perimeter_generator"
            || opt_key == "wall_transition_length"
            || opt_key == "wall_transition_filter_deviation"
            || opt_key == "wall_transition_angle"
            || opt_key == "wall_distribution_count"
            || opt_key == "min_feature_size"
            || opt_key == "min_bead_width") {
            object_steps.insert(posSlice);
        } else if (
               opt_key == "seam_position"
            || opt_key == "scarf_seam_placement"
            || opt_key == "scarf_seam_only_on_smooth"
            || opt_key == "scarf_seam_start_height"
            || opt_key == "scarf_seam_entire_loop"
            || opt_key == "scarf_seam_length"
            || opt_key == "scarf_seam_max_segment_length"
            || opt_key == "scarf_seam_on_inner_perimeters"
            || opt_key == "seam_preferred_direction"
            || opt_key == "seam_preferred_direction_jitter"
            || opt_key == "support_material_speed"
            || opt_key == "support_material_interface_speed"
            || opt_key == "bridge_speed"
            || opt_key == "external_perimeter_speed"
            || opt_key == "small_perimeter_speed"
            || opt_key == "solid_infill_speed"
            || opt_key == "first_layer_infill_speed"
            || opt_key == "top_solid_infill_speed") {
            print_steps.insert(psGCodeExport);
        } else if (
               opt_key == "wipe_into_infill"
            || opt_key == "wipe_into_objects"
            || opt_key == "infill_speed"
            || opt_key == "perimeter_speed") {
            print_steps.insert(psWipeTower);
            print_steps.insert(psGCodeExport);
        } else if (
               opt_key == "enable_dynamic_overhang_speeds"
            || opt_key == "overhang_speed_0"
            || opt_key == "overhang_speed_1"
            || opt_key == "overhang_speed_2"
            || opt_key == "overhang_speed_3") {
            object_steps.insert(posPerimeters);
        } else {
            PANIC("Unknown diff option: " + opt_key);
        }
    }

    return {print_steps, object_steps};
}

const std::set<std::string> options_influencing_gcode{
    "autoemit_temperature_commands",
    "avoid_crossing_perimeters",
    "avoid_crossing_perimeters_max_detour",
    "bed_shape",
    "bed_temperature",
    "before_layer_gcode",
    "between_objects_gcode",
    "binary_gcode",
    "bridge_acceleration",
    "bridge_fan_speed",
    "enable_dynamic_fan_speeds",
    "overhang_fan_speed_0",
    "overhang_fan_speed_1",
    "overhang_fan_speed_2",
    "overhang_fan_speed_3",
    "chamber_temperature",
    "chamber_minimal_temperature",
    "colorprint_heights",
    "cooling",
    "default_acceleration",
    "deretract_speed",
    "disable_fan_first_layers",
    "duplicate_distance",
    "end_gcode",
    "end_filament_gcode",
    "external_perimeter_acceleration",
    "extrusion_axis",
    "extruder_clearance_height",
    "extruder_clearance_radius",
    "extruder_colour",
    "extruder_offset",
    "extrusion_multiplier",
    "fan_always_on",
    "fan_below_layer_time",
    "full_fan_speed_layer",
    "filament_abrasive",
    "filament_colour",
    "filament_diameter",
    "filament_density",
    "filament_notes",
    "filament_cost",
    "filament_seam_gap_distance",
    "filament_spool_weight",
    "first_layer_acceleration",
    "first_layer_acceleration_over_raft",
    "first_layer_bed_temperature",
    "first_layer_speed_over_raft",
    "gcode_comments",
    "gcode_label_objects",
    "nozzle_high_flow",
    "infill_acceleration",
    "layer_gcode",
    "min_fan_speed",
    "max_fan_speed",
    "max_print_height",
    "min_print_speed",
    "max_print_speed",
    "max_volumetric_speed",
    "max_volumetric_extrusion_rate_slope_positive",
    "max_volumetric_extrusion_rate_slope_negative",
    "notes",
    "only_retract_when_crossing_perimeters",
    "output_filename_format",
    "perimeter_acceleration",
    "post_process",
    "gcode_substitutions",
    "printer_notes",
    "travel_ramping_lift",
    "travel_initial_part_length",
    "travel_slope",
    "travel_max_lift",
    "travel_lift_before_obstacle",
    "retract_before_travel",
    "retract_before_wipe",
    "retract_layer_change",
    "retract_length",
    "retract_length_toolchange",
    "retract_lift",
    "retract_lift_above",
    "retract_lift_below",
    "retract_restart_extra",
    "retract_restart_extra_toolchange",
    "retract_speed",
    "seam_gap_distance",
    "single_extruder_multi_material_priming",
    "slowdown_below_layer_time",
    "solid_infill_acceleration",
    "standby_temperature_delta",
    "start_gcode",
    "start_filament_gcode",
    "toolchange_gcode",
    "top_solid_infill_acceleration",
    "travel_acceleration",
    "thumbnails",
    "thumbnails_format",
    "use_firmware_retraction",
    "use_relative_e_distances",
    "use_volumetric_e",
    "variable_layer_height",
    "wipe",
    "wipe_tower_acceleration",
};

const std::set<std::string> options_influencing_skirt_brim{
    "skirts",
    "skirt_height",
    "draft_shield",
    "skirt_distance",
    "min_skirt_length",
    "ooze_prevention",
};

const std::set<std::string> options_influencing_object_slicing{
    "first_layer_height",
    "nozzle_diameter",
    "resolution",
    "spiral_vase",
    "filament_shrinkage_compensation_xy",
    "filament_shrinkage_compensation_z",
    "prefer_clockwise_movements",
};

const std::set<std::string> options_influencing_wipe_tower_and_skir_brim{
    "complete_objects",
    "filament_type",
    "first_layer_temperature",
    "filament_loading_speed",
    "filament_loading_speed_start",
    "filament_unloading_speed",
    "filament_unloading_speed_start",
    "filament_toolchange_delay",
    "filament_cooling_moves",
    "filament_stamping_loading_speed",
    "filament_stamping_distance",
    "filament_minimal_purge_on_wipe_tower",
    "filament_cooling_initial_speed",
    "filament_cooling_final_speed",
    "filament_purge_multiplier",
    "filament_ramming_parameters",
    "filament_multitool_ramming",
    "filament_multitool_ramming_volume",
    "filament_multitool_ramming_flow",
    "filament_max_volumetric_speed",
    "filament_infill_max_speed",
    "filament_infill_max_crossing_speed",
    "gcode_flavor",
    "high_current_on_filament_swap",
    "infill_first",
    "single_extruder_multi_material",
    "temperature",
    "idle_temperature",
    "wipe_tower",
    "wipe_tower_width",
    "wipe_tower_brim_width",
    "wipe_tower_cone_angle",
    "wipe_tower_bridging",
    "wipe_tower_extra_spacing",
    "wipe_tower_extra_flow",
    "wipe_tower_no_sparse_layers",
    "wipe_tower_extruder",
    "wiping_volumes_matrix",
    "wiping_volumes_use_custom_matrix",
    "parking_pos_retraction",
    "cooling_tube_retraction",
    "cooling_tube_length",
    "extra_loading_move",
    "multimaterial_purging",
    "travel_speed",
    "travel_speed_z",
    "first_layer_speed",
    "z_offset",
};

PrintAndObjectSteps diff_to_print_invalidated_steps(const std::vector<std::string>& option_keys)
{
    if (option_keys.empty())
        return {};

    const std::set<std::string> specific_keys{
        "first_layer_extrusion_width",
        "min_layer_height",
        "max_layer_height",
        "gcode_resolution",
    };

    PrintSteps print_steps;
    PrintObjectSteps object_steps;

    for (const std::string& option_key : option_keys) {
        if (options_influencing_gcode.count(option_key) != 0) {
            print_steps.insert(psGCodeExport);
        } else if (options_influencing_skirt_brim.count(option_key) != 0) {
            print_steps.insert(psSkirtBrim);
        } else if (options_influencing_object_slicing.count(option_key) != 0) {
            object_steps.insert(posSlice);
        } else if (options_influencing_wipe_tower_and_skir_brim.count(option_key) != 0) {
            print_steps.insert(psWipeTower);
            print_steps.insert(psSkirtBrim);
        } else if (option_key == "filament_soluble") {
            print_steps.insert(psWipeTower);
            // Soluble support interface / non-soluble base interface produces non-soluble interface layers below soluble interface layers.
            // Thus switching between soluble / non-soluble interface layer material may require recalculation of supports.
            // FIXME Killing supports on any change of "filament_soluble" is rough. We should check for each object whether that is necessary.
            object_steps.insert(posSupportMaterial);
        } else if (specific_keys.count(option_key) != 0) {
            object_steps.insert(posPerimeters);
            object_steps.insert(posInfill);
            object_steps.insert(posSupportMaterial);
            print_steps.insert(psSkirtBrim);
        } else if (option_key == "avoid_crossing_curled_overhangs") {
            object_steps.insert(posEstimateCurledExtrusions);
        } else if (option_key == "automatic_extrusion_widths") {
            object_steps.insert(posPerimeters);
        } else {
            // for legacy, if we can't handle this option let's invalidate all steps
            return {AllSteps{}, AllSteps{}};
            // Continue with the other opt_keys to possibly invalidate any object specific steps.
        }
    }

    return {print_steps, object_steps};
}

PrintAndObjectSteps get_invalidated_steps(const PrintRegion& current, const PrintRegion& next)
{
    const std::vector<std::string> diff{
        current.config().diff_keys(next.config())
    };
    return diff_to_invalidated_steps(current.config(), next.config(), diff);
}

PrintAndObjectSteps merge(const PrintAndObjectSteps& a, const PrintAndObjectSteps& b)
{
    PrintAndObjectSteps result;
    result.first = merge(a.first, b.first);
    result.second = merge(a.second, b.second);
    return result;
}

bool is_all_steps(const PrintAndObjectSteps& steps)
{
    return std::holds_alternative<AllSteps>(steps.first)
        && std::holds_alternative<AllSteps>(steps.second);
}

PrintAndObjectSteps get_invalidated_steps(
    const std::vector<PrintObjectRegions::VolumeRegion>& current_regions,
    const std::vector<PrintObjectRegions::VolumeRegion>& next_regions
)
{
    if (current_regions.size() != next_regions.size()) {
        return {AllSteps{}, AllSteps{}};
    }

    PrintAndObjectSteps result;
    for (std::size_t i{}; i < current_regions.size(); ++i) {
        const PrintObjectRegions::VolumeRegion& current{current_regions[i]};
        const PrintObjectRegions::VolumeRegion& next{next_regions[i]};

        if (current.region == nullptr && next.region == nullptr) {
            continue;
        }

        if (current.region == nullptr || next.region == nullptr) {
            return {AllSteps{}, AllSteps{}};
        }

        const PrintAndObjectSteps invalidated_steps{
            get_invalidated_steps(*current.region, *next.region)
        };

        result = merge(result, invalidated_steps);

        if (is_all_steps(result)) {
            return result;
        }
    }

    return result;
}

PrintAndObjectSteps get_invalidated_steps(
    const std::vector<PrintObjectRegions::PaintedRegion>& current_regions,
    const std::vector<PrintObjectRegions::PaintedRegion>& next_regions
)
{
    if (current_regions.size() != next_regions.size()) {
        return {AllSteps{}, AllSteps{}};
    }

    PrintAndObjectSteps result;
    for (std::size_t i{}; i < current_regions.size(); ++i) {
        const PrintObjectRegions::PaintedRegion& current{current_regions[i]};
        const PrintObjectRegions::PaintedRegion& next{next_regions[i]};

        if (current.parent != next.parent || current.extruder_id != next.extruder_id) {
            return {AllSteps{}, AllSteps{}};
        }

        // ASSERT(current.region != nullptr);
        // ASSERT(next.region != nullptr);
        result = merge(result, get_invalidated_steps(*current.region, *next.region));
        if (is_all_steps(result)) {
            return result;
        }
    }

    return result;
}

PrintAndObjectSteps get_invalidated_steps(
    const std::vector<PrintObjectRegions::FuzzySkinPaintedRegion>& current_regions,
    const std::vector<PrintObjectRegions::FuzzySkinPaintedRegion>& next_regions
)
{
    if (current_regions.size() != next_regions.size()) {
        return {AllSteps{}, AllSteps{}};
    }

    PrintAndObjectSteps result;
    for (std::size_t i{}; i < current_regions.size(); ++i) {
        const PrintObjectRegions::FuzzySkinPaintedRegion& current{current_regions[i]};
        const PrintObjectRegions::FuzzySkinPaintedRegion& next{next_regions[i]};

        if (current.parent != next.parent || current.parent_type != next.parent_type) {
            return {AllSteps{}, AllSteps{}};
        }

        result = merge(result, get_invalidated_steps(*current.region, *next.region));
        if (is_all_steps(result)) {
            return result;
        }
    }

    return result;
}

PrintAndObjectSteps get_invalidated_steps(
    const PrintObjectRegions& current_regions,
    const PrintObjectRegions& next_regions
)
{
    using LayerRangeRegions = PrintObjectRegions::LayerRangeRegions;

    if (current_regions.layer_ranges.size() != next_regions.layer_ranges.size()) {
        return {AllSteps{}, AllSteps{}};
    }

    PrintAndObjectSteps result;
    for (std::size_t index{}; index < current_regions.layer_ranges.size(); ++index) {
        const LayerRangeRegions& current{current_regions.layer_ranges[index]};
        const LayerRangeRegions& next{next_regions.layer_ranges[index]};

        result = merge(result, get_invalidated_steps(current.volume_regions, next.volume_regions));
        if (is_all_steps(result)) {
            return result;
        }

        result = merge(result, get_invalidated_steps(current.painted_regions, next.painted_regions));
        if (is_all_steps(result)) {
            return result;
        }

        result = merge(
            result,
            get_invalidated_steps(current.fuzzy_skin_painted_regions, next.fuzzy_skin_painted_regions)
        );
        if (is_all_steps(result)) {
            return result;
        }
    }

    return result;
}

}
