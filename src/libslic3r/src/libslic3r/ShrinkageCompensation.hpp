#pragma once

#include "Slic3r/Domain/ConfigPack.hpp"
#include "Slic3r/Domain/Model.hpp"

namespace Slic3r::Biz::Slicing {
std::optional<Domain::Vec3d>
get_shrinkage_compensation(const Domain::Model& model, const Domain::ConfigPackFDM& config);
} // namespace Slic3r::Biz::Slicing
