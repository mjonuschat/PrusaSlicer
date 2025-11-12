#pragma once

#include <vector>
#include "Slic3r/Domain/ConfigPack.hpp"
#include "Slic3r/Domain/Model.hpp"

namespace Slic3r::Biz::Slicing {

std::vector<unsigned int> get_painting_extruders(const Domain::ModelObject& model_object);

/* @brief Returns a list of extruder candidates (first extruder is 0).
 *
 * Returns a list of possibly used extruders. Not all of these extruders have to
 * be actually used, but if an extruder is used, it will be returned from this.
 */
std::vector<unsigned>
get_extruder_candidates(const Domain::Model& model, const Domain::ConfigPackFDM& config);

} // namespace Slic3r::Biz::Slicing
