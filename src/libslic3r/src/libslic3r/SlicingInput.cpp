#include "libslic3r/SlicingInput.hpp"

namespace Slic3r {
using Domain::ConfigPackFDM;
using Domain::PartialConfig;
using Domain::FullConfigFDM;
using Domain::FullConfigFDMPtr;
using Domain::ObjectSettings;
using Domain::PartialConfig;
using Domain::PartialObjectConfigFDM;
using Domain::PartialObjectConfigFDMPtr;
using Domain::PartialVolumeConfigFDM;
using Domain::PartialVolumeConfigFDMPtr;
using Domain::VolumeSettings;
using Biz::Slicing::Error;
using Biz::Slicing::ErrorCode;


namespace {

void set_extruders(PartialConfig& partial_config, const auto& settings)
{
    if (const auto extruder{partial_config.template get<int>("extruder")}; extruder > 0) {
        if (!settings.overrides.get("infill_extruder")) {
            partial_config.set("infill_extruder", *extruder);
        }
        if (!settings.overrides.get("perimeter_extruder")) {
            partial_config.set("perimeter_extruder", *extruder);
        }
        if (!settings.overrides.get("solid_infill_extruder")) {
            partial_config.set("solid_infill_extruder", *extruder);
        }
    }
}
} // namespace

tl::expected<FullConfigFDMPtr, std::vector<Error>> prepare_slicing_input(
    const ConfigPackFDM& config_pack,
    const std::vector<unsigned>& extruder_candidates,
    const Domain::Preset::HwPrinterConfig& hw_config
)
{
    std::vector<Error> errors;
    if (hw_config.tools.size() == 0) {
        errors.push_back(Error{ErrorCode::NoHwConfigTools});
    }

    if (hw_config.material_slot_count() < hw_config.tools.size()) {
        errors.push_back(Error{ErrorCode::HwConfigLessMaterialsThanTools});
    }

    for (const Domain::Preset::HwToolConfig& tool : hw_config.tools) {
        if (!tool.features.contains("nozzle_diameter")) {
            errors.push_back(Error{ErrorCode::MissingHwConfigNozzleDiameter});
            break;
        }
    }

    if (!errors.empty()) {
        return tl::unexpected{errors};
    }

    FullConfigFDM result{config_pack, extruder_candidates, hw_config};
    return std::make_shared<const FullConfigFDM>(std::move(result));
}

tl::expected<PartialObjectConfigFDMPtr, std::vector<Error>> prepare_slicing_object_input(
    const ObjectSettings& object_settings,
    const std::size_t material_slot_count
)
{
    PartialObjectConfigFDM result{object_settings, material_slot_count};
    set_extruders(result, object_settings);
    return std::make_shared<PartialObjectConfigFDM>(std::move(result));
};

tl::expected<PartialVolumeConfigFDMPtr, std::vector<Error>> prepare_slicing_volume_input(
    const VolumeSettings& volume_settings,
    const std::size_t material_slot_count
)
{
    PartialVolumeConfigFDM result{volume_settings, material_slot_count};
    set_extruders(result, volume_settings);
    return std::make_shared<const PartialVolumeConfigFDM>(std::move(result));
}
} // namespace Slic3r
