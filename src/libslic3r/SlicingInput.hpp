#pragma once

#include "Slic3r/Domain/ConfigPack.hpp"
#include "Slic3r/Domain/FullConfigFDM.hpp"

namespace Slic3r {
Domain::FullConfigFDMPtr prepare_slicing_input(const Domain::ConfigPackFDM& config_pack);

Domain::PartialObjectConfigFDMPtr prepare_slicing_object_input(
    const Domain::ObjectSettings& object_settings,
    const std::size_t tools_count,
    const std::size_t filaments_count
);

Domain::PartialVolumeConfigFDMPtr prepare_slicing_volume_input(
    const Domain::VolumeSettings& volume_settings,
    const std::size_t tools_count,
    const std::size_t filaments_count
);
} // namespace Slic3r
