#pragma once

#include "Slic3r/Domain/Vectors.hpp"
#include <oneapi/tbb/scalable_allocator.h>

namespace Slic3r::Domain {

static constexpr double SCALING_FACTOR = 0.000001;

//FIXME Better to use an inline function with an explicit return type.
//inline coord_t scale_(double v) { return coord_t(floor(v / SCALING_FACTOR + 0.5f)); }
#define scale_(val) ((val) / SCALING_FACTOR)

class Point : public Vec2crd
{
public:
    using coord_type = coord_t;

    Point() : Vec2crd(0, 0) {}
    using Vec2crd::Vec2crd;

    static Point new_scale(double x, double y) { return Point(coord_t(scale_(x)), coord_t(scale_(y))); }
    template<typename OtherDerived>
    static Point new_scale(const Eigen::MatrixBase<OtherDerived> &v) { return Point(coord_t(scale_(v.x())), coord_t(scale_(v.y()))); }

    // Compatibility with Eigen.
    template<typename OtherDerived>
    Point(const Eigen::MatrixBase<OtherDerived> &other) : Vec2crd(other) {}

    // Compatibility with Eigen.
    template<typename OtherDerived>
    Point& operator=(const Eigen::MatrixBase<OtherDerived> &other)
    {
        this->Vec2crd::operator=(other);
        return *this;
    }

    // These operators address undesired behavior of Eigen (actually c++ in general).
    // Eigen defines operator*(Point, int) which sadly due to c++ implicit
    // conversion can be used with double. It first implicitly narrows the double
    // to int and then does the product. This means that (10, 10) * 1.2 == (10, 10).
    // These operators do the product in doubles and than convert it to int,
    // meaning (10, 10) * 1.2 == (12, 12).
    Point& operator*=(const double &scalar);
    Point operator*(const double &scalar) const;

    void   rotate(double angle) { this->rotate(std::cos(angle), std::sin(angle)); }
    void   rotate(double cos_a, double sin_a) {
        double cur_x = (double)this->x();
        double cur_y = (double)this->y();
        this->x() = (coord_t)round(cos_a * cur_x - sin_a * cur_y);
        this->y() = (coord_t)round(cos_a * cur_y + sin_a * cur_x);
    }

    void   rotate(double angle, const Point &center);
    Point  rotated(double angle) const { Point res(*this); res.rotate(angle); return res; }
    Point  rotated(double cos_a, double sin_a) const { Point res(*this); res.rotate(cos_a, sin_a); return res; }
    Point  rotated(double angle, const Point &center) const { Point res(*this); res.rotate(angle, center); return res; }
};

Vec2crd rotated(const Vec2crd& point, const double angle, const Vec2crd &center = Vec2crd::Zero());

template<typename BaseType>
using PointsAllocator = tbb::scalable_allocator<BaseType>;
using Points          = std::vector<Point, PointsAllocator<Point>>;
}
