#pragma once

#include "Slic3r/Domain/Polygon.hpp"

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

Domain::Polygon scaled(const std::vector<Domain::Vec2d>& points);

} // namespace Slic3r::Biz::Algorithms::Polygon
