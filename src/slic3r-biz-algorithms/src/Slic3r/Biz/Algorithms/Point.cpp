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

} // namespace Slic3r::Biz::Algorithms::Point
