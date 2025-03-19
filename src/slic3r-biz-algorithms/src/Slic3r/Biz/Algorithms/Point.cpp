#include "Slic3r/Biz/Algorithms/Point.hpp"

namespace Slic3r::Biz::Algorithms::Point {

bool has_duplicate_points(const Domain::Points& points)
{
    for (size_t i = 1; i < points.size(); ++i) {
        if (points[i - 1] == points[i]) {
            return true;
        }
    }

    return false;
}

bool remove_duplicate_points(Domain::Points& points)
{
    if (points.empty())
        return false;

    auto last = std::unique(points.begin(), points.end());
    if (last == points.end())
        return false;

    points.erase(last, points.end());
    return true;
}

Domain::Points scaled(const std::vector<Domain::Vec2d>& points)
{
    Domain::Points scaled_points;
    scaled_points.reserve(points.size());
    for (const Domain::Vec2d& pt : points) {
        scaled_points.emplace_back(Domain::Point::new_scale(pt.x(), pt.y()));
    }

    return scaled_points;
}

} // namespace Slic3r::Biz::Algorithms::Point
