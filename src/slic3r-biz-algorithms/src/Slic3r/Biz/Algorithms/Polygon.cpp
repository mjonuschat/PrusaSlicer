#include "Slic3r/Biz/Algorithms/Polygon.hpp"

#include "Slic3r/Biz/Algorithms/Point.hpp"

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

} // namespace Slic3r::Biz::Algorithms::Polygon
