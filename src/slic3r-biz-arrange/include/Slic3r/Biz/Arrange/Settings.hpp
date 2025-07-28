#pragma once

#include <optional>
#include "Slic3r/Biz/Arrange/Bed.hpp"
#include "Slic3r/Biz/Algorithms/Scaling.hpp"

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

enum class Mode
{
    Global,
    Local,
};

struct Settings
{
    Strategy strategy{Strategy::Auto};
    Mode mode{Mode::Global};
    double scaled_offset{0.0};
    double unscaled_bed_offset{0.0};
    bool allow_rotations{false};
    GeometryHandling fixed_geometry{GeometryHandling::Convex};
    GeometryHandling movable_geometry{GeometryHandling::Convex};
    std::optional<PivotPoint> bed_pivot_point;
    std::optional<Domain::Bed::Segments> bed_segments;
    int scaled_simplification_tolerance{Biz::Algorithms::Scaling::scaled(.2)};
};
} // namespace Slic3r::Biz::Arrange
