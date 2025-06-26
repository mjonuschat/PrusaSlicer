#pragma once

#include "Slic3r/Domain/BoundingBox.hpp"
#include "Slic3r/Domain/Line.hpp"
#include "Slic3r/Domain/Point.hpp"
#include "Slic3r/Domain/Types.hpp"

namespace Slic3r::Biz::Algorithms::Line {

double distance_to_squared(const Domain::Line &line, const Domain::Point &point, Domain::Point &nearest_point_out);
double distance_to_squared(const Domain::Line &line, const Domain::Point &point);
double distance_to(const Domain::Line &line, const Domain::Point &point);

/**
 * Returns the squared distance to the nearest point on the infinite segment.
 *
 * @note The nearest point (and returned squared distance to this point) could be beyond the 'a' and 'b' ends of the segment.
 */
double distance_to_infinite_squared(const Domain::Line &line, const Domain::Point &point, Domain::Point &nearest_point_out);

/**
 * Returns the squared distance to the nearest point on the infinite segment.
 *
 * @note The nearest point (and returned squared distance to this point) could be beyond the 'a' and 'b' ends of the segment.
 */
double distance_to_infinite_squared(const Domain::Line &line, const Domain::Point &point);

/**
 * Returns the distance to the nearest point on the infinite segment.
 *
 * @note The nearest point (and returned squared distance to this point) could be beyond the 'a' and 'b' ends of the segment.
 */
double distance_to_infinite(const Domain::Line &line, const Domain::Point &point, Domain::Point &nearest_point_out);

/**
 * Returns the distance to the nearest point on the infinite segment.
 *
 * @note The nearest point (and returned squared distance to this point) could be beyond the 'a' and 'b' ends of the segment.
 */
double distance_to_infinite(const Domain::Line &line, const Domain::Point &point);

bool intersection(const Domain::Line& line, const Domain::Line& other_line, Domain::Point& intersection_point_out);
bool intersection_infinite(const Domain::Line& line, const Domain::Line& other_line, Domain::Point& intersection_point_out);

Domain::BoundingBox2crd get_extents(const Domain::Lines& lines);

Domain::Vec3d intersect_plane(const Domain::Line3d& line, double z);

Domain::Line3d transformed(const Domain::Line3d& line, const Domain::Transform3d& t);

} // namespace Slic3r::Biz::Algorithms::Line
