#pragma once

#include "Slic3r/Domain/Types.hpp"
#include <oneapi/tbb/scalable_allocator.h>

namespace Slic3r::Domain {

static constexpr double SCALING_FACTOR = 0.000001;

//FIXME Better to use an inline function with an explicit return type.
//inline coord_t scale_(double v) { return coord_t(floor(v / SCALING_FACTOR + 0.5f)); }
#define scale_(val) ((val) / Slic3r::Domain::SCALING_FACTOR)

using Point = Vec2crd;

Point rotated(const Point& point, const double angle, const Point &center = Point::Zero());
Point rotated(const Point& point, const double cos_a, const double sin_a);

template<typename BaseType>
using PointsAllocator = tbb::scalable_allocator<BaseType>;
using Points = std::vector<Point, PointsAllocator<Point>>;
}
