#pragma once

#include <optional>
#include "Slic3r/Domain/BoundingBox.hpp"

namespace Slic3r::Biz::Arrange {

enum class Strategy
{
    Auto,
    PullToCenter
};

enum class GeometryHandling
{
    Convex,
    Arbitrary,
};

struct Settings
{
    Strategy strategy{Strategy::Auto};
    bool allow_rotations{false};

    // Used to align rotations if provided. Only useful with rectengular bed.
    std::optional<Domain::BoundingBox2crd> arrangment_limits;

    double scaled_offset{0.0};

    GeometryHandling fixed_geometry{GeometryHandling::Convex};
    GeometryHandling movable_geometry{GeometryHandling::Convex};
};
} // namespace Slic3r::Biz::Arrange
