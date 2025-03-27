#include "Slic3r/Biz/Algorithms/Polygon.hpp"

#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"
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

} // namespace Slic3r::Biz::Algorithms::Polygon
