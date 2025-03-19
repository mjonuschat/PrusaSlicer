#pragma once

#include "Slic3r/Domain/Point.hpp"

namespace Slic3r::Domain {

class Line
{
public:
    static constexpr std::size_t Dim    = 2;
    using                        Scalar = Point::Scalar;

    Point a;
    Point b;

    Line() = default;
    Line(const Point& a, const Point& b) : a(a), b(b) {}

    virtual ~Line() = default;

    bool operator==(const Line& rhs) const;
    bool operator!=(const Line& rhs) const;

    double length() const;
    double orientation() const;
    double direction() const;
    Point midpoint() const;

    Vec2crd vector() const;
    Vec2crd normal() const;

    void scale(double factor);
    void translate(const Point& v);
    void translate(coord_t x, coord_t y);
    void rotate(double angle, const Point& center);
    void reverse();
    void extend(double offset);

    double perp_signed_distance_to(const Point& point) const;
    double perp_distance_to(const Point& point) const;

    bool is_parallel_to(const Line& line) const;
    bool is_parallel_to(double angle) const;
    bool is_perpendicular_to(const Line& line) const;
};

using Lines = std::vector<Line>;

} // namespace Slic3r::Domain
