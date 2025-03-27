#pragma once

#include "Slic3r/Domain/BoundingBox.hpp"
#include "Slic3r/Domain/Line.hpp"
#include "Slic3r/Domain/Polygon.hpp"
#include "Slic3r/Domain/Polyline.hpp"

namespace Slic3r::Biz::Algorithms::Polygon {

/**
 * Checks if the Polygon contains successive duplicate points.
 *
 * @param polygon Polygon to search within.
 * @return true If at least one pair of successive duplicate points is found.
 * @return false Otherwise.
 */
bool has_duplicate_points(const Domain::Polygon& polygon);

/**
 * Removes successive duplicate points from the Polygon.
 *
 * @return true If at least one duplicate point was removed.
 * @return false If no duplicates were found.
 */
bool remove_duplicate_points(Domain::Polygon& polygon);

/**
 * Finds the index of the closest point in the Polygon to the given point.
 *
 * @param polygon Polygon to search within.
 * @param point The point with which to compare distances.
 * @return Index of the closest point in the list, or -1 if the list is empty.
 */
int closest_point_index(const Domain::Polygon& polygon, const Domain::Point& point);

Domain::Polygon scaled(const std::vector<Domain::Vec2d>& points);

Domain::BoundingBox2crd get_bounding_box(const Domain::Polygon& polygon);
Domain::BoundingBox2crd get_bounding_box(const Domain::Polygons& polygons);

bool is_counter_clockwise(const Domain::Polygon& polygon);
bool is_clockwise(const Domain::Polygon& polygon);

bool make_counter_clockwise(Domain::Polygon& polygon);
bool make_clockwise(Domain::Polygon& polygon);

Domain::Polyline split_at_vertex(const Domain::Polygon& polygon, const Domain::Point& point);

/**
 * Split a closed polygon into an open polyline, with the split point duplicated at both ends.
 */
Domain::Polyline split_at_index(const Domain::Polygon& polygon, size_t index);

Domain::Polyline split_at_first_point(const Domain::Polygon& polygon);

Domain::Lines to_lines(const Domain::Polygon& polygon);
Domain::Lines to_lines(const Domain::Polygons& polygons);

} // namespace Slic3r::Biz::Algorithms::Polygon
