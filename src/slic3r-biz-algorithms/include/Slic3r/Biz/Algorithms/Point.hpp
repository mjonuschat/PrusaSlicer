#pragma once

#include "Slic3r/Domain/Point.hpp"

namespace Slic3r::Biz::Algorithms::Point {

/**
 * Checks if the Points contains successive duplicate points.
 *
 * @param points Points to search within.
 * @return true If at least one pair of successive duplicate points is found.
 * @return false Otherwise.
 */
bool has_duplicate_points(const Domain::Points& points);

/**
 * Removes successive duplicate points from the Points.
 *
 * @return true If at least one duplicate point was removed.
 * @return false If no duplicates were found.
 */
bool remove_duplicate_points(Domain::Points& points);

} // namespace Slic3r::Biz::Algorithms::Point
