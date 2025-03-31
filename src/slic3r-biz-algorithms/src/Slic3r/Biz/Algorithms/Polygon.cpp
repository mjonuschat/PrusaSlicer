#include "Slic3r/Biz/Algorithms/Polygon.hpp"

#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"
#include "Slic3r/Biz/Algorithms/DouglasPeucker.hpp"
#include "Slic3r/Biz/Algorithms/Point.hpp"

// FIXME: Temporarily use includes from libslic3r until Clipper integration refactoring.
#include <libslic3r/ClipperUtils.hpp>

using namespace Slic3r::Biz::Algorithms;

namespace Slic3r::Biz::Algorithms::Polygon {

namespace Impl {

size_t count_points(const Domain::Polygons& polys)
{
    size_t n_points = 0;
    for (const auto& poly : polys) {
        n_points += poly.size();
    }

    return n_points;
}

} // namespace Impl

bool has_duplicate_points(const Domain::Polygon& polygon)
{
    return Point::has_duplicate_points(polygon.points);
}

bool remove_duplicate_points(Domain::Polygon& polygon)
{
    return Point::remove_duplicate_points(polygon.points);
}

int closest_point_index(const Domain::Polygon& polygon, const Domain::Point& point)
{
    return Slic3r::Biz::Algorithms::MultiPoint::closest_point_index(polygon, point);
}

Domain::Polygon scaled(const std::vector<Domain::Vec2d>& points)
{
    return Domain::Polygon(Point::scaled(points));
}

// TODO: Uncomment after migration to BoundingBox2crd.
/*
Domain::BoundingBox2crd get_bounding_box(const Domain::Polygon& polygon)
{
    return BoundingBox::construct(polygon.points);
}

Domain::BoundingBox2crd get_bounding_box(const Domain::Polygons& polygons)
{
    Domain::BoundingBox2crd bounding_box;
    for (const Domain::Polygon& polygon : polygons) {
        bounding_box = BoundingBox::merge(bounding_box, get_bounding_box(polygon));
    }

    return bounding_box;
}*/

bool is_counter_clockwise(const Domain::Polygon& polygon)
{
    return ClipperLib::Orientation(polygon.points);
}

bool is_clockwise(const Domain::Polygon& polygon)
{
    return !Polygon::is_counter_clockwise(polygon);
}

bool make_counter_clockwise(Domain::Polygon& polygon)
{
    if (!Polygon::is_counter_clockwise(polygon)) {
        polygon.reverse();
        return true;
    }

    return false;
}

bool make_clockwise(Domain::Polygon& polygon)
{
    if (Polygon::is_counter_clockwise(polygon)) {
        polygon.reverse();
        return true;
    }
    return false;
}

Domain::Polyline split_at_vertex(const Domain::Polygon& polygon, const Domain::Point& point)
{
    // Find index of point.
    for (const Domain::Point& pt : polygon.points) {
        if (pt == point) {
            const size_t pt_idx = &pt - &polygon.points.front();
            return Polygon::split_at_index(polygon, pt_idx);
        }
    }

    throw Slic3r::InvalidArgument("Point not found");
    return {};
}

Domain::Polyline split_at_index(const Domain::Polygon& polygon, const size_t index)
{
    Domain::Polyline polyline;
    polyline.points.reserve(polygon.points.size() + 1);
    for (Points::const_iterator it = polygon.points.begin() + static_cast<int>(index); it != polygon.points.end(); ++it) {
        polyline.points.push_back(*it);
    }

    for (Points::const_iterator it = polygon.points.begin(); it != polygon.points.begin() + static_cast<int>(index) + 1; ++it) {
        polyline.points.push_back(*it);
    }

    return polyline;
}

Domain::Polyline split_at_first_point(const Domain::Polygon& polygon)
{
    return Polygon::split_at_index(polygon, 0);
}

std::optional<Domain::Point> intersection(const Domain::Polygon& polygon, const Domain::Line& line)
{
    if (polygon.points.size() < 2)
        return std::nullopt;

    Domain::Point intersection_pt;
    if (Algorithms::Line::intersection(Domain::Line(polygon.points.front(), polygon.points.back()), line, intersection_pt))
        return intersection_pt;

    for (size_t idx = 1; idx < polygon.points.size(); ++idx) {
        if (Algorithms::Line::intersection(Domain::Line(polygon.points[idx - 1], polygon.points[idx]), line, intersection_pt))
            return intersection_pt;
    }

    return std::nullopt;
}

Domain::Polygons simplify(const Domain::Polygon& polygon, double tolerance)
{
    // Works on CCW polygons only, CW contour will be reoriented to CCW by Clipper's simplify_polygons()!
    assert(Polygon::is_counter_clockwise(polygon));

    // Repeat the first point at the end in order to apply Douglas-Peucker on the whole polygon.
    Points points = polygon.points;
    points.push_back(points.front());
    Domain::Polygon p(Algorithms::DouglasPeucker::douglas_peucker(points, tolerance));
    p.points.pop_back();

    Domain::Polygons pp;
    pp.push_back(p);
    return simplify_polygons(pp);
}

bool contains(const Domain::Polygon& polygon, const Domain::Point& point, const bool border_result)
{
    if (const int poly_count_inside = ClipperLib::PointInPolygon(point, polygon.points); poly_count_inside == -1) {
        return border_result;
    } else {
        return (poly_count_inside % 2) == 1;
    }
}

bool contains(const Domain::Polygons& polygons, const Domain::Point& point, const bool border_result)
{
    int poly_count_inside = 0;
    for (const Domain::Polygon& polygon : polygons) {
        const int is_inside_this_poly = ClipperLib::PointInPolygon(point, polygon.points);
        if (is_inside_this_poly == -1)
            return border_result;

        poly_count_inside += is_inside_this_poly;
    }

    return (poly_count_inside % 2) == 1;
}

Domain::Lines to_lines(const Domain::Polygon& polygon)
{
    if (polygon.size() < 3)
        return {};

    Domain::Lines lines;
    lines.reserve(polygon.size());
    for (size_t idx = 1; idx < polygon.size(); ++idx) {
        lines.emplace_back(polygon.points[idx - 1], polygon.points[idx]);
    }

    lines.emplace_back(polygon.points.back(), polygon.points.front());

    return lines;
}

Domain::Lines to_lines(const Domain::Polygons& polygons)
{
    Domain::Lines lines;
    lines.reserve(Impl::count_points(polygons));
    for (const Domain::Polygon& polygon : polygons) {
        for (size_t idx = 1; idx < polygon.size(); ++idx) {
            lines.emplace_back(polygon.points[idx - 1], polygon.points[idx]);
        }

        lines.emplace_back(polygon.points.back(), polygon.points.front());
    }

    return lines;
}

Domain::BoundingBox2crd get_extents(const Domain::Polygon& poly)
{
    const Domain::BoundingBox2crd bbox = Algorithms::BoundingBox::construct(poly.points);
    return Slic3r::BoundingBox{bbox.min, bbox.max};
}

Domain::BoundingBox2crd get_extents(const Domain::Polygons &polygons)
{
    Domain::BoundingBox2crd bb;
    if (! polygons.empty()) {
        bb = get_extents(polygons.front());
        for (size_t i = 1; i < polygons.size(); ++ i)
            bb = BoundingBox::merge(bb, get_extents(polygons[i]));
    }
    return bb;
}

} // namespace Slic3r::Biz::Algorithms::Polygon
