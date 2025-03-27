#include "Slic3r/Biz/Algorithms/Polyline.hpp"

#include "Slic3r/Domain/BoundingBox.hpp"
#include "Slic3r/Domain/Point.hpp"
#include "Slic3r/Domain/Line.hpp"
#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"
#include "Slic3r/Biz/Algorithms/Point.hpp"

namespace Slic3r::Biz::Algorithms::Polyline {

using namespace Slic3r::Domain;
using namespace Slic3r::Biz::Algorithms;

void reverse(Domain::Polyline& polyline)
{
    std::reverse(polyline.points.begin(), polyline.points.end());
}

Domain::Polyline reversed(const Domain::Polyline& polyline)
{
    Domain::Polyline polyline_out = polyline;
    reverse(polyline_out);
    return polyline_out;
}

bool has_duplicate_points(const Domain::Polyline& polyline)
{
    return Point::has_duplicate_points(polyline.points);
}

bool remove_duplicate_points(Domain::Polyline& polyline)
{
    return Point::remove_duplicate_points(polyline.points);
}

void clip_end(Domain::Polyline& polyline, double distance)
{
    while (distance > 0) {
        const Vec2d last_point = polyline.last_point().cast<double>();

        polyline.points.pop_back();
        if (polyline.points.empty())
            break;

        const Vec2d v = polyline.last_point().cast<double>() - last_point;
        const double lsqr = v.squaredNorm();
        if (lsqr > distance * distance) {
            polyline.points.emplace_back((last_point + v * (distance / sqrt(lsqr))).cast<coord_t>());
            return;
        }

        distance -= std::sqrt(lsqr);
    }
}

void clip_start(Domain::Polyline& polyline, double distance)
{
    Polyline::reverse(polyline);
    Polyline::clip_end(polyline, distance);

    if (polyline.points.size() >= 2) {
        Polyline::reverse(polyline);
    }
}

void extend_end(Domain::Polyline& polyline, double distance)
{
    const Vec2d v = (polyline.points.back() - *(polyline.points.end() - 2)).cast<double>().normalized();
    polyline.points.back() += (v * distance).cast<coord_t>();
}

void extend_start(Domain::Polyline& polyline, double distance)
{
    const Vec2d v = (polyline.points.front() - polyline.points[1]).cast<double>().normalized();
    polyline.points.front() += (v * distance).cast<coord_t>();
}

Domain::Polyline scaled(const std::vector<Domain::Vec2d>& points)
{
    return Domain::Polyline(Point::scaled(points));
}

Domain::BoundingBox2crd get_bounding_box(const Domain::Polyline& polyline)
{
    return BoundingBox::construct(polyline.points);
}

Domain::BoundingBox2crd get_bounding_box(const Domain::Polylines& polylines)
{
    Domain::BoundingBox2crd bounding_box;
    for (const Domain::Polyline& polyline : polylines) {
        bounding_box = BoundingBox::merge(bounding_box, get_bounding_box(polyline));
    }

    return bounding_box;
}

Domain::Lines to_lines(const Domain::Polyline& polyline)
{
    if (polyline.points.size() < 2)
        return {};

    Lines lines;
    lines.reserve(polyline.points.size() - 1);
    for (Points::const_iterator it = polyline.points.begin(); it != polyline.points.end() - 1; ++it) {
        lines.emplace_back(*it, *(it + 1));
    }

    return lines;
}

Domain::Lines to_lines(const Domain::Polylines& polylines)
{
    const size_t lines_cnt = total_lines_count(polylines);

    Lines lines;
    lines.reserve(lines_cnt);
    for (const Domain::Polyline& polyline : polylines) {
        for (Points::const_iterator it = polyline.points.begin(); it != polyline.points.end() - 1; ++it) {
            lines.emplace_back(*it, *(it + 1));
        }
    }

    return lines;
}

double total_length(const Domain::Polylines& polylines)
{
    double total_length = 0;
    for (const Domain::Polyline& pl : polylines) {
        total_length += pl.length();
    }

    return total_length;
}

size_t total_lines_count(const Domain::Polylines& polylines)
{
    size_t lines_cnt = 0;
    for (const Domain::Polyline& polyline : polylines) {
        if (polyline.points.size() > 1) {
            lines_cnt += polyline.points.size() - 1;
        }
    }

    return lines_cnt;
}

} // namespace Slic3r::Biz::Algorithms::Polyline
