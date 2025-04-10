#include "Slic3r/Biz/Algorithms/Point.hpp"
#include "Slic3r/Biz/Algorithms/Scaling.hpp"

namespace Slic3r::Biz::Algorithms::Point {

bool has_consecutive_duplicate_points(const Domain::Points& points)
{
    return std::adjacent_find(points.begin(), points.end()) != points.end();
}

bool remove_consecutive_duplicate_points(Domain::Points& points, const bool check_first_and_last)
{
    if (points.empty())
        return false;

    auto last_it = std::unique(points.begin(), points.end());

    // If requested, check if the first and last points are duplicates and remove the last one if true.
    if (check_first_and_last && last_it != points.begin() && points.front() == *(last_it - 1)) {
        --last_it;
    }

    if (last_it == points.end())
        return false;

    points.erase(last_it, points.end());
    return true;
}

Domain::Points scaled(const std::vector<Domain::Vec2d>& points)
{
    Domain::Points scaled_points;
    scaled_points.reserve(points.size());
    for (const Domain::Vec2d& pt : points) {
        scaled_points.emplace_back(Scaling::scaled(Domain::Vec2d{pt.x(), pt.y()}));
    }

    return scaled_points;
}
} // namespace Slic3r::Biz::Algorithms::Point
