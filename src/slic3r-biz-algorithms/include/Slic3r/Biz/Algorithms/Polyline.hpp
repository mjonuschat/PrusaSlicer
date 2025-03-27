#pragma once

#include "Slic3r/Domain/BoundingBox.hpp"
#include "Slic3r/Domain/Line.hpp"
#include "Slic3r/Domain/Polyline.hpp"

namespace Slic3r::Biz::Algorithms::Polyline {

void reverse(Domain::Polyline& polyline);

Domain::Polyline reversed(const Domain::Polyline& polyline);

/**
 * Finds the index of the closest point to the query point within a given epsilon.
 *
 * @param polyline Polyline to search within.
 * @param query_pt The point to find the closest match for.
 * @param scaled_epsilon The maximum allowed epsilon for a match.
 * @return Index of the closest point if found, otherwise -1.
 */
int find_point(const Domain::Polyline& polyline, const Domain::Point& query_pt, double scaled_epsilon);

/**
 * Checks if the Polyline contains successive duplicate points.
 *
 * @param polyline Polyline to search within.
 * @return true If at least one pair of successive duplicate points is found.
 * @return false Otherwise.
 */
bool has_duplicate_points(const Domain::Polyline& polyline);

/**
 * Removes successive duplicate points from the Polyline.
 *
 * @return true If at least one duplicate point was removed.
 * @return false If no duplicates were found.
 */
bool remove_duplicate_points(Domain::Polyline& polyline);

/**
 * @brief Trims the given distance from the end of the polyline.
 *
 * @param polyline The polyline to clip.
 * @param distance The distance to remove from the end.
 */
void clip_end(Domain::Polyline& polyline, double distance);

/**
 * @brief Trims the given distance from the start of the polyline.
 *
 * @param polyline The polyline to clip.
 * @param distance The distance to remove from the start.
 */
void clip_start(Domain::Polyline& polyline, double distance);

/**
 * @brief Extends the last segment of the polyline by given distance.
 *
 * @param polyline The polyline to extend.
 * @param distance The distance to extend the last segment.
 */
void extend_end(Domain::Polyline& polyline, double distance);

/**
 * @brief Extends the first segment of the polyline by given distance.
 *
 * @param polyline The polyline to extend.
 * @param distance The distance to extend the first segment.
 */
void extend_start(Domain::Polyline& polyline, double distance);

Domain::Polyline scaled(const std::vector<Domain::Vec2d> &points);

Domain::BoundingBox2crd get_bounding_box(const Domain::Polyline& polyline);
Domain::BoundingBox2crd get_bounding_box(const Domain::Polylines& polylines);

Domain::Lines to_lines(const Domain::Polyline& polyline);
Domain::Lines to_lines(const Domain::Polylines& polylines);

double total_length(const Domain::Polylines& polylines);
size_t total_lines_count(const Domain::Polylines& polylines);

bool is_straight(const Domain::Polyline& polyline);

void simplify(Domain::Polyline& polyline, double tolerance);
Domain::Polyline simplified(const Domain::Polyline& polyline, double tolerance);

std::pair<Domain::Polyline, Domain::Polyline> split_at_point(const Domain::Polyline& polyline, const Domain::Point& split_point);

} // namespace Slic3r::Biz::Algorithms::Polyline
