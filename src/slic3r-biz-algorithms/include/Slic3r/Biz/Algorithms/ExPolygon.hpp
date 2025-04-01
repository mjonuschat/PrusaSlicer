#pragma once

#include "Slic3r/Domain/ExPolygon.hpp"
#include "Slic3r/Domain/Line.hpp"
#include "Slic3r/Domain/Polygon.hpp"
#include "Slic3r/Domain/Polyline.hpp"

namespace Slic3r::Biz::Algorithms::ExPolygon {

bool is_valid(const Domain::ExPolygon& expolygon);

size_t count_points(const Domain::ExPolygon& expolygon);
size_t count_points(const Domain::ExPolygons& expolygons);

bool contains(const Domain::ExPolygon& expolygon, const Domain::Point& point, bool border_result = true);
bool contains(const Domain::ExPolygons& expolygons, const Domain::Point& point, bool border_result = true);

bool contains(const Domain::ExPolygon& expolygon, const Domain::Line& line);
bool contains(const Domain::ExPolygon& expolygon, const Domain::Polyline& polyline);
bool contains(const Domain::ExPolygon& expolygon, const Domain::Polylines& polylines);

// Does this expolygon overlap another expolygon?
// Either the ExPolygons intersect, or one is fully inside the other,
// and it is not inside a hole of the other expolygon.
// The test may not be commutative if the two expolygons touch by a boundary only,
// see unit test SCENARIO("Clipper diff with polyline", "[Clipper]").
// Namely expolygons touching at a vertical boundary are considered overlapping, while expolygons touching
// at a horizontal boundary are NOT considered overlapping.
bool overlaps(const Domain::ExPolygon& expolygon, const Domain::ExPolygon &other_expolygon);

Domain::Lines to_lines(const Domain::ExPolygon& expolygon);
Domain::Lines to_lines(const Domain::ExPolygons& expolygons);

Domain::ExPolygons simplify(const Domain::ExPolygon& expolygon, double tolerance);
Domain::Polygons simplify_to_polygons(const Domain::ExPolygon& expolygon, double tolerance);

} // namespace Slic3r::Biz::Algorithms::ExPolygon
