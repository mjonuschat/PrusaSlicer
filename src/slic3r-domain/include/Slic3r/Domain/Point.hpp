#pragma once

#include "Slic3r/Domain/Vectors.hpp"
#include <oneapi/tbb/scalable_allocator.h>

namespace Slic3r::Domain {

static constexpr double SCALING_FACTOR = 0.000001;

//FIXME Better to use an inline function with an explicit return type.
//inline coord_t scale_(double v) { return coord_t(floor(v / SCALING_FACTOR + 0.5f)); }
#define scale_(val) ((val) / Slic3r::Domain::SCALING_FACTOR)

class Point : public Vec2crd
{
public:
    using coord_type = coord_t;

    // Eigen vectors are not 0 initialized. Fix that.
    Point() : Vec2crd(0, 0) {}
    using Vec2crd::Vec2crd;
    using Vec2crd::operator=;

    // These operators address undesired behavior of Eigen (actually c++ in general).
    // Eigen defines operator*(Point, int) which sadly due to c++ implicit
    // conversion can be used with double. It first implicitly narrows the double
    // to int and then does the product. This means that (10, 10) * 1.2 == (10, 10).
    // These operators do the product in doubles and than convert it to int,
    // meaning (10, 10) * 1.2 == (12, 12).
    Point& operator*=(const double &scalar);
    Point operator*(const double &scalar) const;
};

Point rotated(const Point& point, const double angle, const Point &center = Point::Zero());
Point rotated(const Point& point, const double cos_a, const double sin_a);

template<typename BaseType>
using PointsAllocator = tbb::scalable_allocator<BaseType>;
using Points = std::vector<Point, PointsAllocator<Point>>;
}
