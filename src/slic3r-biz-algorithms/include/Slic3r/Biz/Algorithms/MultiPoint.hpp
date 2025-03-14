#pragma once

#include "Slic3r/Domain/MultiPoint.hpp"

namespace Slic3r::Biz::Algorithms::MultiPoint {

void reverse(Domain::MultiPoint& multi_point);

Domain::MultiPoint reversed(const Domain::MultiPoint& multi_point);

/**
 * Finds the index of the closest point to the query point within a given epsilon.
 *
 * @param multi_point MultiPoint to search within.
 * @param query_pt The point to find the closest match for.
 * @param scaled_epsilon The maximum allowed epsilon for a match.
 * @return Index of the closest point if found, otherwise -1.
 */
int find_point(const Domain::MultiPoint& multi_point, const Domain::Point& query_pt, double scaled_epsilon);

/**
 * Checks if the MultiPoint contains successive duplicate points.
 *
 * @param multi_point MultiPoint to search within.
 * @return true If at least one pair of successive duplicate points is found.
 * @return false Otherwise.
 */
bool has_duplicate_points(const Domain::MultiPoint& multi_point);

/**
 * Removes successive duplicate points from the MultiPoint.
 *
 * @return true If at least one duplicate point was removed.
 * @return false If no duplicates were found.
 */
bool remove_duplicate_points(Domain::MultiPoint& multi_point);

/**
 * Finds the index of the closest point in the MultiPoint to the given point.
 *
 * @param multi_point MultiPoint to search within.
 * @param point The point with which to compare distances.
 * @return Index of the closest point in the list, or -1 if the list is empty.
 */
int closest_point_index(const Domain::MultiPoint& multi_point, const Domain::Point& point);

} // namespace Slic3r::Biz::Algorithms::MultiPoint
