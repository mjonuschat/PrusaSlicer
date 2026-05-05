#pragma once

#include <vector>
#include "Slic3r/Domain/BedInstance.hpp"
#include "Slic3r/Domain/ConfigPack.hpp"
#include "Slic3r/Domain/Model.hpp"

namespace Slic3r::Biz::Slicing {

std::vector<unsigned int>
get_painting_extruders(const Domain::ModelObject& model_object, unsigned int num_extruders);

std::set<unsigned> get_volume_extruder_candidates(
    const Domain::VolumeSettings& volume_settings,
    const Domain::ObjectSettings& object_settings,
    const Domain::PrintSettings& print_settings
);

std::set<unsigned>
get_object_extruder_candidates(const Domain::ModelObject& object, const Domain::ConfigPackFDM& config);

/* @brief Returns a list of extruder candidates (first extruder is 0).
 *
 * Returns a list of possibly used extruders. Not all of these extruders have to
 * be actually used, but if an extruder is used, it will be returned from this.
 */
std::vector<unsigned> get_extruder_candidates(
    const Domain::Model& model,
    const Domain::ConfigPackFDM& config,
    const Domain::BedInstance& bed
);

} // namespace Slic3r::Biz::Slicing
