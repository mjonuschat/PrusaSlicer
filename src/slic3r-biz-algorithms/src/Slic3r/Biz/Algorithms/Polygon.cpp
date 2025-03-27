#include "Slic3r/Biz/Algorithms/Polygon.hpp"

#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"
#include "Slic3r/Biz/Algorithms/Point.hpp"

// FIXME: Temporarily use includes from libslic3r until Clipper integration refactoring.
#include <libslic3r/ClipperUtils.hpp>

namespace Slic3r::Biz::Algorithms::Polygon {

using namespace Slic3r::Biz::Algorithms;

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
}

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

} // namespace Slic3r::Biz::Algorithms::Polygon
