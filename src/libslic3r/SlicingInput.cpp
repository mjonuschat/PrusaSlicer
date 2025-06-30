#include "libslic3r/SlicingInput.hpp"
#include <ranges>

namespace Slic3r {
using Domain::SquashedConfig;
using Domain::ConfigPackFDM;
using Domain::PartialConfig;
using Domain::FullConfig;
using Domain::FullConfigFDM;
using Domain::FullConfigFDMPtr;
using Domain::ObjectSettings;
using Domain::PartialConfig;
using Domain::PartialObjectConfigFDM;
using Domain::PartialObjectConfigFDMPtr;
using Domain::PartialVolumeConfigFDM;
using Domain::PartialVolumeConfigFDMPtr;
using Domain::VolumeSettings;
using Domain::ConfigValue;
using Domain::EnumVectorWrapper;
using Domain::EnumWrapper;


namespace {
namespace ToolSettings {
    const std::vector<std::string> object_and_volume_overrides{
        "bottom_solid_layers",
        "bottom_solid_min_thickness",
        "bridge_flow_ratio",
        "bridge_speed",
        "elefant_foot_compensation",
        "enable_dynamic_overhang_speeds",
        "external_perimeter_extrusion_width",
        "external_perimeter_speed",
        "fill_density",
        "fill_pattern",
        "gap_fill_enabled",
        "gap_fill_speed",
        "infill_anchor",
        "infill_anchor_max",
        "infill_every_layers",
        "infill_extrusion_width",
        "infill_overlap",
        "infill_speed",
        "overhang_speed_0",
        "overhang_speed_1",
        "overhang_speed_2",
        "overhang_speed_3",
        "overhangs",
        "perimeter_extrusion_width",
        "perimeter_speed",
        "perimeters",
        "small_perimeter_speed",
        "solid_infill_extrusion_width",
        "solid_infill_speed",
        "top_infill_extrusion_width",
        "top_solid_infill_speed",
        "top_solid_layers",
        "top_solid_min_thickness",
    };
    const std::vector<std::string> object_overrides{
        "brim_separation",
        "dont_support_bridges",
        "extrusion_width",
        "first_layer_acceleration_over_raft",
        "first_layer_speed_over_raft",
        "min_bead_width",
        "min_feature_size",
        "raft_contact_distance",
        "raft_expansion",
        "raft_first_layer_density",
        "raft_first_layer_expansion",
        "seam_position",
        "support_material",
        "support_material_angle",
        "support_material_auto",
        "support_material_bottom_contact_distance",
        "support_material_bottom_interface_layers",
        "support_material_buildplate_only",
        "support_material_closing_radius",
        "support_material_contact_distance",
        "support_material_enforce_layers",
        "support_material_extruder",
        "support_material_extrusion_width",
        "support_material_interface_contact_loops",
        "support_material_interface_extruder",
        "support_material_interface_layers",
        "support_material_interface_pattern",
        "support_material_interface_spacing",
        "support_material_interface_speed",
        "support_material_pattern",
        "support_material_spacing",
        "support_material_speed",
        "support_material_style",
        "support_material_synchronize_layers",
        "support_material_threshold",
        "support_material_with_sheath",
        "support_material_xy_spacing",
        "support_tree_angle",
        "support_tree_angle_slow",
        "support_tree_branch_diameter",
        "support_tree_branch_diameter_angle",
        "support_tree_branch_diameter_double_wall",
        "support_tree_branch_distance",
        "support_tree_tip_diameter",
        "support_tree_top_rate",
        "thick_bridges",
    };
    const std::vector<std::string> no_overrides{
        "bridge_acceleration",
        "default_acceleration",
        "external_perimeter_acceleration",
        "first_layer_acceleration",
        "first_layer_extrusion_width",
        "first_layer_infill_speed",
        "first_layer_speed",
        "gcode_resolution",
        "infill_acceleration",
        "max_print_speed",
        "max_volumetric_extrusion_rate_slope_negative",
        "max_volumetric_extrusion_rate_slope_positive",
        "max_volumetric_speed",
        "only_retract_when_crossing_perimeters",
        "over_bridge_speed",
        "perimeter_acceleration",
        "solid_infill_acceleration",
        "top_solid_infill_acceleration",
        "travel_acceleration",
        "travel_speed",
        "travel_speed_z",
        "wipe_tower_extra_flow",
        "wipe_tower_extra_spacing",
    };
};

void replace_impl(const ConfigValue value, const std::string& key, SquashedConfig& config) {
    value.visit([&](auto&& value){
        using ValueType = std::remove_cvref_t<decltype(value)>;
        if constexpr (Domain::is_std_vector_v<ValueType>) {
            ASSERT(!value.empty());
            config.set(key, value.front());
        } else if constexpr (std::is_same_v<ValueType, EnumVectorWrapper>) {
            ASSERT(!value.values().empty());
            const EnumWrapper enum_value{value.values().front(), value.type(), value.def()};
            config.set(key, enum_value);
        }
    });
}

void replace_with_first_element(const std::string& key, FullConfig& full_config) {
    const ConfigValue value{full_config.values().at(key)};
    replace_impl(value, key, full_config);
}

void replace_with_first_element(const std::string& key, PartialConfig& partial_config) {
    const auto it{partial_config.values().find(key)};
    if (it == partial_config.values().end()) {
        return;
    }
    const ConfigValue value{it->second};
    replace_impl(value, key, partial_config);
}

void transform_to_legacy_input(FullConfig& full_config) {
    const auto keys{std::ranges::join_view(
        std::vector<std::vector<std::string>>{
            ToolSettings::object_and_volume_overrides,
            ToolSettings::object_overrides,
            ToolSettings::no_overrides,
        }
    )};
    for (const std::string& key : keys) {
        replace_with_first_element(key, full_config);
    }
}

void transform_to_legacy_input(PartialObjectConfigFDM& object_config) {
    const auto keys{std::ranges::join_view(
        std::vector<std::vector<std::string>>{
            ToolSettings::object_and_volume_overrides,
            ToolSettings::object_overrides,
        }
    )};
    for (const std::string& key : keys) {
        replace_with_first_element(key, object_config);
    }
}

void transform_to_legacy_input(PartialVolumeConfigFDM& volume_config) {
    const auto keys{std::ranges::join_view(
        std::vector<std::vector<std::string>>{
            ToolSettings::object_and_volume_overrides,
        }
    )};
    for (const std::string& key : keys) {
        replace_with_first_element(key, volume_config);
    }
}

void set_extruders(PartialConfig& partial_config)
{
    if (const auto extruder{partial_config.template get<int>("extruder")}; extruder > 0) {
        partial_config.set("infill_extruder", *extruder);
        partial_config.set("perimeter_extruder", *extruder);
        partial_config.set("solid_infill_extruder", *extruder);
    }
}
} // namespace

FullConfigFDMPtr prepare_slicing_input(const ConfigPackFDM& config_pack)
{
    FullConfigFDM result{config_pack};
    transform_to_legacy_input(result);
    return std::make_shared<const FullConfigFDM>(std::move(result));
}

PartialObjectConfigFDMPtr prepare_slicing_object_input(
    const ObjectSettings& object_settings,
    const std::size_t tools_count,
    const std::size_t filaments_count
)
{
    PartialObjectConfigFDM result{object_settings, tools_count, filaments_count};
    set_extruders(result);
    transform_to_legacy_input(result);
    return std::make_shared<PartialObjectConfigFDM>(std::move(result));
};

PartialVolumeConfigFDMPtr prepare_slicing_volume_input(
    const VolumeSettings& volume_settings,
    const std::size_t tools_count,
    const std::size_t filaments_count
)
{
    PartialVolumeConfigFDM result{volume_settings, tools_count, filaments_count};
    set_extruders(result);
    transform_to_legacy_input(result);
    return std::make_shared<const PartialVolumeConfigFDM>(std::move(result));
}
} // namespace Slic3r
