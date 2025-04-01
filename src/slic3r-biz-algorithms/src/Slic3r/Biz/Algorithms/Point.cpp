#include "Slic3r/Biz/Algorithms/Point.hpp"
#include "Slic3r/Biz/Algorithms/Scaling.hpp"

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
        scaled_points.emplace_back(Scaling::scaled(Domain::Vec2d{pt.x(), pt.y()}));
    }

    return scaled_points;
}

template<UnscaledVector2 Vec>
typename Vec::Scalar cross2(const Vec &v1, const Vec &v2)
{
    return v1.x() * v2.y() - v1.y() * v2.x();
}

template double cross2<Domain::Vec2d>(const Domain::Vec2d &v1, const Domain::Vec2d &v2);
template float cross2<Domain::Vec2f>(const Domain::Vec2f &v1, const Domain::Vec2f &v2);

} // namespace Slic3r::Biz::Algorithms::Point
