#pragma once

#include "Slic3r/Domain/BoundingBox.hpp"
#include "Slic3r/Domain/Line.hpp"
#include "Slic3r/Domain/Polyline.hpp"

namespace Slic3r::Biz::Algorithms::Polyline {

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

} // namespace Slic3r::Biz::Algorithms::Polyline
