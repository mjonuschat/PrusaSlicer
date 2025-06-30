#include "libslic3r/SlicingInput.hpp"

namespace Slic3r {
using Domain::ConfigPackFDM;
using Domain::FullConfigFDM;
using Domain::FullConfigFDMPtr;
using Domain::ObjectSettings;
using Domain::PartialConfig;
using Domain::PartialObjectConfigFDM;
using Domain::PartialObjectConfigFDMPtr;
using Domain::PartialVolumeConfigFDM;
using Domain::PartialVolumeConfigFDMPtr;
using Domain::VolumeSettings;

FullConfigFDMPtr prepare_slicing_input(const ConfigPackFDM& config_pack)
{
    FullConfigFDM result{config_pack};
    return std::make_shared<const FullConfigFDM>(std::move(result));
}

namespace {
void set_extruders(PartialConfig& partial_config)
{
    if (const auto extruder{partial_config.template get<int>("extruder")}; extruder > 0) {
        partial_config.set("infill_extruder", *extruder);
        partial_config.set("perimeter_extruder", *extruder);
        partial_config.set("solid_infill_extruder", *extruder);
    }
}
} // namespace

PartialVolumeConfigFDMPtr prepare_slicing_volume_input(
    const VolumeSettings& volume_settings,
    const std::size_t tools_count,
    const std::size_t filaments_count
)
{
    PartialVolumeConfigFDM result{volume_settings, tools_count, filaments_count};
    set_extruders(result);
    return std::make_shared<const PartialVolumeConfigFDM>(std::move(result));
}

PartialObjectConfigFDMPtr prepare_slicing_object_input(
    const ObjectSettings& object_settings,
    const std::size_t tools_count,
    const std::size_t filaments_count
)
{
    PartialObjectConfigFDM result{object_settings, tools_count, filaments_count};
    set_extruders(result);
    return std::make_shared<PartialObjectConfigFDM>(std::move(result));
};
} // namespace Slic3r
