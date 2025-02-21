#pragma once

#include "Slic3r/Domain/Vectors.hpp"

namespace Slic3r::Domain {

static constexpr double SCALING_FACTOR = 0.000001;

//FIXME Better to use an inline function with an explicit return type.
//inline coord_t scale_(coordf_t v) { return coord_t(floor(v / SCALING_FACTOR + 0.5f)); }
#define scale_(val) ((val) / SCALING_FACTOR)

class Point : public Vec2crd
{
public:
    using coord_type = coord_t;

    Point() : Vec2crd(0, 0) {}
    Point(int32_t x, int32_t y) : Vec2crd(coord_t(x), coord_t(y)) {}
    Point(int64_t x, int64_t y) : Vec2crd(coord_t(x), coord_t(y)) {}
    Point(double x, double y) : Vec2crd(coord_t(std::round(x)), coord_t(std::round(y))) {}
    Point(const Point &rhs) { *this = rhs; }
	explicit Point(const Vec2d& rhs) : Vec2crd(coord_t(std::round(rhs.x())), coord_t(std::round(rhs.y()))) {}
	// This constructor allows you to construct Point from Eigen expressions
    // This constructor has to be implicit (non-explicit) to allow implicit conversion from Eigen expressions.
    template<typename OtherDerived>
    Point(const Eigen::MatrixBase<OtherDerived> &other) : Vec2crd(other) {}
    static Point new_scale(coordf_t x, coordf_t y) { return Point(coord_t(scale_(x)), coord_t(scale_(y))); }
    template<typename OtherDerived>
    static Point new_scale(const Eigen::MatrixBase<OtherDerived> &v) { return Point(coord_t(scale_(v.x())), coord_t(scale_(v.y()))); }

    // This method allows you to assign Eigen expressions to MyVectorType
    template<typename OtherDerived>
    Point& operator=(const Eigen::MatrixBase<OtherDerived> &other)
    {
        this->Vec2crd::operator=(other);
        return *this;
    }

    Point& operator+=(const Point& rhs) { this->x() += rhs.x(); this->y() += rhs.y(); return *this; }
    Point& operator-=(const Point& rhs) { this->x() -= rhs.x(); this->y() -= rhs.y(); return *this; }
	Point& operator*=(const double &rhs) { this->x() = coord_t(this->x() * rhs); this->y() = coord_t(this->y() * rhs); return *this; }
    Point operator*(const double &rhs) const { return Point(this->x() * rhs, this->y() * rhs); }

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
}
