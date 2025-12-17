#pragma once

#include <optional>
#include "Slic3r/Domain/Types.hpp"
#include "libslic3r/ConfigViews.hpp"

namespace Slic3r::Biz::Slicing {
std::optional<Domain::Vec3d> get_shrinkage_compensation(
    const std::vector<unsigned int>& extruders,
    const PrintConfigView& config
);
} // namespace Slic3r::Biz::Slicing
