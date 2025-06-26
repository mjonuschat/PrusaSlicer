///|/ Copyright (c) Prusa Research 2023 Tomáš Mészáros @tamasmeszaros
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef BEDS_HPP
#define BEDS_HPP

#include <boost/variant.hpp>
#include <boost/variant/variant.hpp>
#include <numeric>
#include <numbers>
#include <cmath>
#include <limits>
#include <type_traits>

#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"
#include "Slic3r/Biz/Algorithms/ClipperUtils.hpp"
#include "Slic3r/Biz/Algorithms/ExPolygon.hpp"
#include "Slic3r/Biz/Algorithms/Polygon.hpp"
#include "Slic3r/Biz/Algorithms/Scaling.hpp"
#include "Slic3r/Domain/BoundingBox.hpp"
#include "Slic3r/Domain/ExPolygon.hpp"
#include "Slic3r/Domain/Point.hpp"
#include "Slic3r/Domain/Polygon.hpp"
#include "Slic3r/Domain/Types.hpp"

namespace Slic3r { namespace arr2 {

// Bed types to be used with arrangement. Most generic bed is a simple polygon
// with holes, but other special bed types are also valid, like a bed without
// boundaries, or a special case of a rectangular or circular bed which leaves
// a lot of room for optimizations.

// Representing an unbounded bed.
struct InfiniteBed {
    Domain::Point center;
    explicit InfiniteBed(const Domain::Point &p = {0, 0}): center{p} {}
};

Domain::BoundingBox2crd bounding_box(const InfiniteBed &bed);

inline InfiniteBed offset(const InfiniteBed &bed, Domain::coord_t) { return bed; }
inline Domain::Vec2crd bed_gap(const InfiniteBed &)
{
    return Domain::Vec2crd::Zero();
}

struct RectangleBed {
    Domain::BoundingBox2crd bb;
    Domain::Vec2crd gap;

    explicit RectangleBed(const Domain::BoundingBox2crd &bedbb, const Domain::Vec2crd &gap) : bb{bedbb}, gap{gap} {}
    explicit RectangleBed(Domain::coord_t w, Domain::coord_t h, const Domain::Vec2crd &gap = Domain::Vec2crd::Zero(), Domain::Point c = {0, 0}):
        bb{{c.x() - w / 2, c.y() - h / 2}, {c.x() + w / 2, c.y() + h / 2}}, gap{gap}
    {}

    Domain::coord_t width() const { return Biz::Algorithms::BoundingBox::sizes(bb).x(); }
    Domain::coord_t height() const { return Biz::Algorithms::BoundingBox::sizes(bb).y(); }
};

inline Domain::BoundingBox2crd bounding_box(const RectangleBed &bed) { return bed.bb; }
inline RectangleBed offset(RectangleBed bed, Domain::coord_t v)
{
    bed.bb = Biz::Algorithms::BoundingBox::inflated(bed.bb, v);
    return bed;
}
inline Domain::Vec2crd bed_gap(const RectangleBed &bed) {
    return bed.gap;
}

Domain::Polygon to_rectangle(const Domain::BoundingBox2crd &bb);

inline Domain::Polygon to_rectangle(const RectangleBed &bed)
{
    return to_rectangle(bed.bb);
}

class CircleBed {
    Domain::Point  m_center;
    double m_radius;
    Domain::Vec2crd m_gap;

public:
    CircleBed(): m_center(0, 0), m_radius(std::numeric_limits<double>::quiet_NaN()), m_gap(Domain::Vec2crd::Zero()) {}
    explicit CircleBed(const Domain::Point& c, double r, const Domain::Vec2crd &g)
        : m_center(c)
        , m_radius(r)
        , m_gap(g)
    {}

    double radius() const { return m_radius; }
    const Domain::Point& center() const { return m_center; }
    const Domain::Vec2crd &gap() const { return m_gap; }
};

// Function to approximate a circle with a convex polygon
Domain::Polygon approximate_circle_with_polygon(const CircleBed &bed, int nedges = 24);

inline Domain::BoundingBox2crd bounding_box(const CircleBed &bed)
{
    auto r = static_cast<Domain::coord_t>(std::round(bed.radius()));
    Domain::Point R{r, r};

    return {bed.center() - R, bed.center() + R};
}
inline CircleBed offset(const CircleBed &bed, Domain::coord_t v)
{
    return CircleBed{bed.center(), bed.radius() + v, bed.gap()};
}
inline Domain::Vec2crd bed_gap(const CircleBed &bed)
{
    return bed.gap();
}

struct IrregularBed { Domain::ExPolygons poly; Domain::Vec2crd gap; };
inline Domain::BoundingBox2crd bounding_box(const IrregularBed &bed)
{
    return Biz::Algorithms::ExPolygon::get_extents(bed.poly);
}

inline IrregularBed offset(IrregularBed bed, Domain::coord_t v)
{
    bed.poly = Biz::Algorithms::ClipperUtils::offset_ex(bed.poly, v);
    return bed;
}
inline Domain::Vec2crd bed_gap(const IrregularBed &bed)
{
    return bed.gap;
}

using ArrangeBed =
    boost::variant<InfiniteBed, RectangleBed, CircleBed, IrregularBed>;

inline Domain::BoundingBox2crd bounding_box(const ArrangeBed &bed)
{
    Domain::BoundingBox2crd ret;
    auto visitor = [&ret](const auto &b) { ret = bounding_box(b); };
    boost::apply_visitor(visitor, bed);

    return ret;
}

inline ArrangeBed offset(ArrangeBed bed, Domain::coord_t v)
{
    auto visitor = [v](auto &b) { b = offset(b, v); };
    boost::apply_visitor(visitor, bed);

    return bed;
}

inline Domain::Vec2crd bed_gap(const ArrangeBed &bed)
{
    Domain::Vec2crd ret;
    auto visitor = [&ret](const auto &b) { ret = bed_gap(b); };
    boost::apply_visitor(visitor, bed);

    return ret;
}

inline double area(const Domain::BoundingBox2crd &bb)
{
    auto bbsz = Biz::Algorithms::BoundingBox::sizes(bb);
    return double(bbsz.x()) * bbsz.y();
}

inline double area(const RectangleBed &bed)
{
    auto bbsz = Biz::Algorithms::BoundingBox::sizes(bed.bb);
    return double(bbsz.x()) * bbsz.y();
}

inline double area(const InfiniteBed &bed)
{
    return std::numeric_limits<double>::infinity();
}

inline double area(const IrregularBed &bed)
{
    return std::accumulate(bed.poly.begin(), bed.poly.end(), 0.,
                           [](double s, auto &p) { return s + p.area(); });
}

inline double area(const CircleBed &bed)
{
    return bed.radius() * bed.radius() * std::numbers::pi;
}

inline double area(const ArrangeBed &bed)
{
    double ret = 0.;
    auto visitor = [&ret](auto &b) { ret = area(b); };
    boost::apply_visitor(visitor, bed);

    return ret;
}

inline Domain::ExPolygons to_expolygons(const InfiniteBed &bed)
{
    return {Domain::ExPolygon{to_rectangle(RectangleBed{Biz::Algorithms::Scaling::scaled(1000.), Biz::Algorithms::Scaling::scaled(1000.)})}};
}

inline Domain::ExPolygons to_expolygons(const RectangleBed &bed)
{
    return {Domain::ExPolygon{to_rectangle(bed)}};
}

inline Domain::ExPolygons to_expolygons(const CircleBed &bed)
{
    return {Domain::ExPolygon{approximate_circle_with_polygon(bed)}};
}

inline Domain::ExPolygons to_expolygons(const IrregularBed &bed) { return bed.poly; }

inline Domain::ExPolygons to_expolygons(const ArrangeBed &bed)
{
    Domain::ExPolygons ret;
    auto visitor = [&ret](const auto &b) { ret = to_expolygons(b); };
    boost::apply_visitor(visitor, bed);

    return ret;
}

ArrangeBed to_arrange_bed(const Domain::Points &bedpts, const Domain::Vec2crd &gap);

template<class Bed, class En = void> struct IsRectangular_ : public std::false_type {};
template<> struct IsRectangular_<RectangleBed>: public std::true_type {};
template<> struct IsRectangular_<Domain::BoundingBox2crd>: public std::true_type {};

template<class Bed> static constexpr bool IsRectangular = IsRectangular_<Bed>::value;

} // namespace arr2

inline Domain::BoundingBox2crd &bounding_box(Domain::BoundingBox2crd &bb) { return bb; }
inline const Domain::BoundingBox2crd &bounding_box(const Domain::BoundingBox2crd &bb) { return bb; }
inline Domain::BoundingBox2crd bounding_box(const Domain::Polygon &p) { return Biz::Algorithms::Polygon::get_extents(p); }

} // namespace Slic3r

#endif // BEDS_HPP
