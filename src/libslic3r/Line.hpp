///|/ Copyright (c) Prusa Research 2016 - 2023 Tomáš Mészáros @tamasmeszaros, Vojtěch Bubník @bubnikv, Pavel Mikuš @Godrak, Lukáš Hejl @hejllukas, Filip Sykala @Jony01, Enrico Turri @enricoturri1966
///|/ Copyright (c) 2017 Eyal Soha @eyal0
///|/ Copyright (c) Slic3r 2013 - 2016 Alessandro Ranellucci @alranel
///|/
///|/ ported from lib/Slic3r/Line.pm:
///|/ Copyright (c) Prusa Research 2022 Vojtěch Bubník @bubnikv
///|/ Copyright (c) Slic3r 2011 - 2014 Alessandro Ranellucci @alranel
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_Line_hpp_
#define slic3r_Line_hpp_

#include <type_traits>
#include <cmath>
#include <utility>
#include <vector>
#include <complex>

#include "Slic3r/Domain/Line.hpp"
#include "Slic3r/Biz/Algorithms/Line.hpp"
#include "libslic3r.h"
#include "Point.hpp"

namespace Slic3r {

using Line = Slic3r::Domain::Line;
using Lines = Slic3r::Domain::Lines;

class BoundingBox;
class Linef3;
class ThickLine;

typedef std::vector<ThickLine> ThickLines;

Linef3 transform(const Linef3& line, const Transform3d& t);

namespace line_alg {

template<class L, class En = void> struct Traits {
    static constexpr int Dim = L::Dim;
    using Scalar = typename L::Scalar;

    static LegacyVec<Dim, Scalar>& get_a(L &l) { return l.a; }
    static LegacyVec<Dim, Scalar>& get_b(L &l) { return l.b; }
    static const LegacyVec<Dim, Scalar>& get_a(const L &l) { return l.a; }
    static const LegacyVec<Dim, Scalar>& get_b(const L &l) { return l.b; }
};

template<class L> const constexpr int Dim = Traits<remove_cvref_t<L>>::Dim;
template<class L> using Scalar = typename Traits<remove_cvref_t<L>>::Scalar;

template<class L> auto get_a(L &&l) { return Traits<remove_cvref_t<L>>::get_a(l); }
template<class L> auto get_b(L &&l) { return Traits<remove_cvref_t<L>>::get_b(l); }

// Distance to the closest point of line.
template<class L>
inline double distance_to_squared(const L &line, const LegacyVec<Dim<L>, Scalar<L>> &point, LegacyVec<Dim<L>, Scalar<L>> *nearest_point)
{
    using VecType = LegacyVec<Dim<L>, double>;
    const VecType  v  = (get_b(line) - get_a(line)).template cast<double>();
    const VecType  va = (point  - get_a(line)).template cast<double>();
    const double  l2 = v.squaredNorm();
    if (l2 == 0.0) {
        // a == b case
        *nearest_point = get_a(line);
        return va.squaredNorm();
    }
    // Consider the line extending the segment, parameterized as a + t (b - a).
    // We find projection of this point onto the line.
    // It falls where t = [(this-a) . (b-a)] / |b-a|^2
    const double t = va.dot(v);
    if (t <= 0.0) {
        // beyond the 'a' end of the segment
        *nearest_point = get_a(line);
        return va.squaredNorm();
    } else if (t >= l2) {
        // beyond the 'b' end of the segment
        *nearest_point = get_b(line);
        return (point - get_b(line)).template cast<double>().squaredNorm();
    }

    const VecType w = ((t / l2) * v).eval();
    *nearest_point = (get_a(line).template cast<double>() + w).template cast<Scalar<L>>();
    return (w - va).squaredNorm();
}

// Distance to the closest point of line.
template<class L>
double distance_to_squared(const L &line, const LegacyVec<Dim<L>, Scalar<L>> &point)
{
    LegacyVec<Dim<L>, Scalar<L>> nearest_point;
    return distance_to_squared<L>(line, point, &nearest_point);
}

template<class L>
double distance_to(const L &line, const LegacyVec<Dim<L>, Scalar<L>> &point)
{
    return std::sqrt(distance_to_squared(line, point));
}

// Returns a squared distance to the closest point on the infinite.
// Returned nearest_point (and returned squared distance to this point) could be beyond the 'a' and 'b' ends of the segment.
template<class L>
double distance_to_infinite_squared(const L &line, const LegacyVec<Dim<L>, Scalar<L>> &point, LegacyVec<Dim<L>, Scalar<L>> *closest_point)
{
    const LegacyVec<Dim<L>, double> v  = (get_b(line) - get_a(line)).template cast<double>();
    const LegacyVec<Dim<L>, double> va = (point - get_a(line)).template cast<double>();
    const double              l2 = v.squaredNorm(); // avoid a sqrt
    if (l2 == 0.) {
        // a == b case
        *closest_point = get_a(line);
        return va.squaredNorm();
    }
    // Consider the line extending the segment, parameterized as a + t (b - a).
    // We find projection of this point onto the line.
    // It falls where t = [(this-a) . (b-a)] / |b-a|^2
    const double t = va.dot(v) / l2;
    *closest_point = (get_a(line).template cast<double>() + t * v).template cast<Scalar<L>>();
    return (t * v - va).squaredNorm();
}

// Returns a squared distance to the closest point on the infinite.
// Closest point (and returned squared distance to this point) could be beyond the 'a' and 'b' ends of the segment.
template<class L>
double distance_to_infinite_squared(const L &line, const LegacyVec<Dim<L>, Scalar<L>> &point)
{
    LegacyVec<Dim<L>, Scalar<L>> nearest_point;
    return distance_to_infinite_squared<L>(line, point, &nearest_point);
}

template<class L> bool intersection(const L &l1, const L &l2, LegacyVec<Dim<L>, Scalar<L>> *intersection_pt)
{
    using Floating      = typename std::conditional<std::is_floating_point<Scalar<L>>::value, Scalar<L>, double>::type;
    using VecType       = const LegacyVec<Dim<L>, Floating>;
    const VecType v1    = (l1.b - l1.a).template cast<Floating>();
    const VecType v2    = (l2.b - l2.a).template cast<Floating>();
    Floating      denom = cross2(v1, v2);
    if (fabs(denom) < EPSILON)
#if 0
        // Lines are collinear. Return true if they are coincident (overlappign).
        return ! (fabs(nume_a) < EPSILON && fabs(nume_b) < EPSILON);
#else
        return false;
#endif
    const VecType v12 = (l1.a - l2.a).template cast<Floating>();
    Floating nume_a = cross2(v2, v12);
    Floating nume_b = cross2(v1, v12);
    Floating t1     = nume_a / denom;
    Floating t2     = nume_b / denom;
    if (t1 >= 0 && t1 <= 1.0f && t2 >= 0 && t2 <= 1.0f) {
        // Get the intersection point.
        (*intersection_pt) = (l1.a.template cast<Floating>() + t1 * v1).template cast<Scalar<L>>();
        return true;
    }
    return false; // not intersecting
}

inline Point midpoint(const Point &a, const Point &b) {
    return (a + b) / 2;
}

} // namespace line_alg

class ThickLine : public Line
{
public:
    ThickLine() : a_width(0), b_width(0) {}
    ThickLine(const Point& a, const Point& b) : Line(a, b), a_width(0), b_width(0) {}
    ThickLine(const Point& a, const Point& b, double wa, double wb) : Line(a, b), a_width(wa), b_width(wb) {}

    double a_width, b_width;
};

class CurledLine : public Line
{
public:
    CurledLine() : curled_height(0.0f) {}
    CurledLine(const Point& a, const Point& b) : Line(a, b), curled_height(0.0f) {}
    CurledLine(const Point& a, const Point& b, float curled_height) : Line(a, b), curled_height(curled_height) {}

    float curled_height;
};

using CurledLines = std::vector<CurledLine>;

class Linef
{
public:
    Linef() : a(Vec2d::Zero()), b(Vec2d::Zero()) {}
    Linef(const Vec2d& _a, const Vec2d& _b) : a(_a), b(_b) {}
    virtual ~Linef() = default;

    Vec2d a;
    Vec2d b;

    static const constexpr int Dim = 2;
    using Scalar = Vec2d::Scalar;
};
using Linesf = std::vector<Linef>;

class Linef3
{
public:
    Linef3() : a(Vec3d::Zero()), b(Vec3d::Zero()) {}
    Linef3(const Vec3d& _a, const Vec3d& _b) : a(_a), b(_b) {}

    Vec3d   intersect_plane(double z) const;
    void    scale(double factor) { this->a *= factor; this->b *= factor; }
    Vec3d   vector() const { return this->b - this->a; }
    double  length() const { return vector().norm(); }

    Vec3d a;
    Vec3d b;

    static const constexpr int Dim = 3;
    using Scalar = Vec3d::Scalar;
};

BoundingBox get_extents(const Lines &lines);

} // namespace Slic3r

// start Boost
#include <boost/polygon/polygon.hpp>

namespace boost { namespace polygon {
    template <>
    struct geometry_concept<Slic3r::Line> { typedef segment_concept type; };

    template <>
    struct segment_traits<Slic3r::Line> {
        typedef coord_t coordinate_type;
        typedef Slic3r::Point point_type;
    
        static inline point_type get(const Slic3r::Line& line, direction_1d dir) {
            return dir.to_int() ? line.b : line.a;
        }
    };
} }
// end Boost

#endif // slic3r_Line_hpp_
