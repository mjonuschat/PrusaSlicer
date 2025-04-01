///|/ Copyright (c) Prusa Research 2016 - 2023 Tomáš Mészáros @tamasmeszaros, Vojtěch Bubník @bubnikv, Lukáš Matěna @lukasmatena, Lukáš Hejl @hejllukas, Filip Sykala @Jony01, Oleksandra Iushchenko @YuSanka
///|/ Copyright (c) Slic3r 2013 - 2016 Alessandro Ranellucci @alranel
///|/
///|/ ported from lib/Slic3r/Polygon.pm:
///|/ Copyright (c) Prusa Research 2017 - 2022 Vojtěch Bubník @bubnikv
///|/ Copyright (c) Slic3r 2011 - 2014 Alessandro Ranellucci @alranel
///|/ Copyright (c) 2012 Mark Hindess
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_Polygon_hpp_
#define slic3r_Polygon_hpp_

#include <assert.h>
#include <math.h>
#include <oneapi/tbb/scalable_allocator.h>
#include <vector>
#include <string>
#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <utility>
#include <cassert>
#include <cmath>

#include "Slic3r/Domain/MultiPoint.hpp"
#include "Slic3r/Domain/Point.hpp"
#include "Slic3r/Domain/Polygon.hpp"
#include "libslic3r.h"
#include "Line.hpp"
#include "Point.hpp"
#include "MultiPoint.hpp"
#include "Polyline.hpp"

namespace Slic3r {

using Polygon = Slic3r::Domain::Polygon;
using Polygons = Slic3r::Domain::Polygons;

class BoundingBox;
class ColorPolygon;

using ColorPolygons = std::vector<ColorPolygon>;

// Considering CCW orientation of this polygon, find all convex resp. concave points
// with the angle at the vertex larger than a threshold.
// Zero angle_threshold means to accept all convex resp. concave points.
Points convex_points(const Polygon &polygon, double angle_threshold = 0.);
Points concave_points(const Polygon &polygon, double angle_threshold = 0.);
// Projection of a point onto the polygon.
Point point_projection(const Polygon &polygon, const Point &point);

// Approximate on boundary test.
inline bool polygon_on_boundary(const Polygon &polygon, const Point &point, double eps)
{
    return (point_projection(polygon, point) - point).cast<double>().squaredNorm() < eps * eps;
}

BoundingBox get_extents(const Polygon &poly);
BoundingBox get_extents(const Polygons &polygons);
BoundingBox get_extents_rotated(const Polygon &poly, double angle);
std::vector<BoundingBox> get_extents_vector(const Polygons &polygons);

// Polygon must be valid (at least three points), collinear points and duplicate points removed.
bool        polygon_is_convex(const Points &poly);
inline bool polygon_is_convex(const Polygon &poly) { return polygon_is_convex(poly.points); }

// Test for duplicate points. The points are copied, sorted and checked for duplicates globally.
inline bool has_duplicate_points(Polygon &&poly)      { return has_duplicate_points(std::move(poly.points)); }
inline bool has_duplicate_points(const Polygon &poly) { return has_duplicate_points(poly.points); }
bool        has_duplicate_points(const Polygons &polys);

// Return True when erase some otherwise False.
bool remove_same_neighbor(Polygon &polygon);
bool remove_same_neighbor(Polygons &polygons);

inline double total_length(const Polygons &polylines) {
    double total = 0;
    for (Polygons::const_iterator it = polylines.begin(); it != polylines.end(); ++it)
        total += it->length();
    return total;
}

inline double area(const Polygon &poly) { return poly.area(); }

inline double area(const Polygons &polys)
{
    double s = 0.;
    for (auto &p : polys) s += p.area();

    return s;
}

// Remove sticks (tentacles with zero area) from the polygon.
bool remove_sticks(Polygon &poly);
bool remove_sticks(Polygons &polys);

// Remove polygons with less than 3 edges.
bool remove_degenerate(Polygons &polys);
bool remove_small(Polygons &polys, double min_area);
void remove_collinear(Polygon &poly);
void remove_collinear(Polygons &polys);

Polygons polygons_simplify(Polygons &&polys, double tolerance, bool strictly_simple = true);
Polygons polygons_simplify(const Polygons &polys, double tolerance, bool strictly_simple = true);

inline void polygons_rotate(Polygons &polys, double angle)
{
    const double cos_angle = cos(angle);
    const double sin_angle = sin(angle);
    for (Polygon &p : polys)
        p.rotate(cos_angle, sin_angle);
}

inline void polygons_reverse(Polygons &polys)
{
    for (Polygon &p : polys)
        p.reverse();
}

inline Points to_points(const Polygon &poly)
{
    return poly.points;
}

inline size_t count_points(const Polygons &polys) {
    size_t n_points = 0;
    for (const auto &poly: polys) n_points += poly.points.size();
    return n_points;
}

inline Points to_points(const Polygons &polys) 
{
    Points points;
    points.reserve(count_points(polys));
    for (const Polygon &poly : polys)
        append(points, poly.points);
    return points;
}

inline Lines to_lines(const Polygon &poly) 
{
    Lines lines;
    lines.reserve(poly.points.size());
    if (poly.points.size() > 2) {
        for (Points::const_iterator it = poly.points.begin(); it != poly.points.end()-1; ++it)
            lines.push_back(Line(*it, *(it + 1)));
        lines.push_back(Line(poly.points.back(), poly.points.front()));
    }
    return lines;
}

inline Lines to_lines(const Polygons &polys) 
{
    Lines lines;
    lines.reserve(count_points(polys));
    for (size_t i = 0; i < polys.size(); ++ i) {
        const Polygon &poly = polys[i];
        for (Points::const_iterator it = poly.points.begin(); it != poly.points.end()-1; ++it)
            lines.push_back(Line(*it, *(it + 1)));
        lines.push_back(Line(poly.points.back(), poly.points.front()));
    }
    return lines;
}

inline Polyline to_polyline(const Polygon &polygon)
{
    Polyline out;
    out.points.reserve(polygon.size() + 1);
    out.points.assign(polygon.points.begin(), polygon.points.end());
    out.points.push_back(polygon.points.front());
    return out;
}

inline Polylines to_polylines(const Polygons &polygons)
{
    Polylines out;
    out.reserve(polygons.size());
    for (const Polygon &polygon : polygons)
        out.emplace_back(to_polyline(polygon));
    return out;
}

inline Polylines to_polylines(Polygons &&polys)
{
    Polylines polylines;
    polylines.assign(polys.size(), Polyline());
    size_t idx = 0;
    for (auto it = polys.begin(); it != polys.end(); ++ it) {
        Polyline &pl = polylines[idx ++];
        pl.points = std::move(it->points);
        pl.points.push_back(pl.points.front());
    }
    assert(idx == polylines.size());
    return polylines;
}

// close polyline to polygon (connect first and last point in polyline)
inline Polygons to_polygons(const Polylines &polylines)
{
    Polygons out;
    out.reserve(polylines.size());
    for (const Polyline &polyline : polylines) {
        if (polyline.size())
        out.emplace_back(polyline.points);
    }
    return out;
}

inline Polygons to_polygons(const VecOfPoints &paths)
{
    Polygons out;
    out.reserve(paths.size());
    for (const Points &path : paths)
        out.emplace_back(path);
    return out;
}

inline Polygons to_polygons(VecOfPoints &&paths)
{
    Polygons out;
    out.reserve(paths.size());
    for (Points &path : paths)
        out.emplace_back(std::move(path));
    return out;
}

// Do polygons match? If they match, they must have the same topology,
// however their contours may be rotated.
bool polygons_match(const Polygon &l, const Polygon &r);

Polygon make_circle(double radius, double error);
Polygon make_circle_num_segments(double radius, size_t num_segments);

/// <summary>
/// Define point laying on polygon
/// keep index of polygon line and point coordinate
/// </summary>
struct PolygonPoint
{
    // index of line inside of polygon
    // 0 .. from point polygon[0] to polygon[1]
    size_t index;

    // Point, which lay on line defined by index
    Point point;
};
using PolygonPoints = std::vector<PolygonPoint>;

// To replace reserve_vector where it's used for Polygons
template<class I> IntegerOnly<I, Polygons> reserve_polygons(I cap)
{
    return reserve_vector<Polygon, I, typename Polygons::allocator_type>(cap);
}

class ColorPolygon : public Polygon
{
public:
    using Color  = uint8_t;
    using Colors = std::vector<Color>;

    Colors colors;

    ColorPolygon() = default;
    explicit ColorPolygon(const Points &points, const Colors &colors) : Polygon(points), colors(colors) {}
    ColorPolygon(std::initializer_list<Point> points, std::initializer_list<Color> colors) : Polygon(points), colors(colors) {}
    ColorPolygon(const ColorPolygon &other) : ColorPolygon(other.points, other.colors) {}
    ColorPolygon(ColorPolygon &&other) noexcept : ColorPolygon(std::move(other.points), std::move(other.colors)) {}
    ColorPolygon(Points &&points, Colors &&colors) : Polygon(std::move(points)), colors(std::move(colors)) {}

    void reverse() override {
        Polygon::reverse();
        std::reverse(this->colors.begin(), this->colors.end());
    }

    ColorPolygon &operator=(const ColorPolygon &other) {
        this->points = other.points;
        this->colors = other.colors;
        return *this;
    }

    BoundingBox bounding_box() const;
};

using ColorPolygons = std::vector<ColorPolygon>;

} // namespace Slic3r

// start Boost
#include <boost/polygon/polygon.hpp>

namespace boost::polygon {
    template <>
    struct geometry_concept<Slic3r::Polygon>{ typedef polygon_concept type; };

    template <>
    struct polygon_traits<Slic3r::Polygon> {
        typedef coord_t coordinate_type;
        typedef Slic3r::Points::const_iterator iterator_type;
        typedef Slic3r::Point point_type;

        // Get the begin iterator
        static inline iterator_type begin_points(const Slic3r::Polygon& t) {
            return t.points.begin();
        }

        // Get the end iterator
        static inline iterator_type end_points(const Slic3r::Polygon& t) {
            return t.points.end();
        }

        // Get the number of sides of the polygon
        static inline std::size_t size(const Slic3r::Polygon& t) {
            return t.points.size();
        }

        // Get the winding direction of the polygon
        static inline winding_direction winding(const Slic3r::Polygon& /* t */) {
            return unknown_winding;
        }
    };

    template <>
    struct polygon_mutable_traits<Slic3r::Polygon> {
        // expects stl style iterators
        template <typename iT>
        static inline Slic3r::Polygon& set_points(Slic3r::Polygon& polygon, iT input_begin, iT input_end) {
            polygon.points.clear();
            while (input_begin != input_end) {
                polygon.points.push_back(Slic3r::Point());
                boost::polygon::assign(polygon.points.back(), *input_begin);
                ++input_begin;
            }
            // skip last point since Boost will set last point = first point
            polygon.points.pop_back();
            return polygon;
        }
    };
    
    template <>
    struct geometry_concept<Slic3r::Polygons> { typedef polygon_set_concept type; };

    //next we map to the concept through traits
    template <>
    struct polygon_set_traits<Slic3r::Polygons> {
        typedef coord_t coordinate_type;
        typedef Slic3r::Polygons::const_iterator iterator_type;
        typedef Slic3r::Polygons operator_arg_type;

        static inline iterator_type begin(const Slic3r::Polygons& polygon_set) {
            return polygon_set.begin();
        }

        static inline iterator_type end(const Slic3r::Polygons& polygon_set) {
            return polygon_set.end();
        }

        //don't worry about these, just return false from them
        static inline bool clean(const Slic3r::Polygons& /* polygon_set */) { return false; }
        static inline bool sorted(const Slic3r::Polygons& /* polygon_set */) { return false; }
    };

    template <>
    struct polygon_set_mutable_traits<Slic3r::Polygons> {
        template <typename input_iterator_type>
        static inline void set(Slic3r::Polygons& polygons, input_iterator_type input_begin, input_iterator_type input_end) {
          polygons.assign(input_begin, input_end);
        }
    };
} // namespace boost::polygon
// end Boost

namespace Slic3r::Biz::Algorithms::Polygon {

// TODO: Temporary proxy method that will be removed after migration to BoundingBox2crd.
Slic3r::BoundingBox get_bounding_box(const Slic3r::Polygon& poly);

} // namespace Slic3r::Biz::Algorithms::Polygon

#endif
