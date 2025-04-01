#include "Slic3r/Biz/Algorithms/ExPolygon.hpp"

#include "Slic3r/Biz/Algorithms/DouglasPeucker.hpp"
#include "Slic3r/Biz/Algorithms/Polygon.hpp"
#include "Slic3r/Utils.hpp"

// FIXME: Temporarily use includes from libslic3r until Clipper integration refactoring.
#include <libslic3r/ClipperUtils.hpp>

using namespace Slic3r::Biz::Algorithms;

namespace Slic3r::Biz::Algorithms::ExPolygon {

bool is_valid(const Domain::ExPolygon& expolygon)
{
    if (!expolygon.contour.is_valid() || !Algorithms::Polygon::is_counter_clockwise(expolygon.contour))
        return false;

    for (const Domain::Polygon& hole : expolygon.holes) {
        if (!hole.is_valid() || Algorithms::Polygon::is_counter_clockwise(hole))
            return false;
    }

    return true;
}

size_t count_points(const Domain::ExPolygon& expolygon)
{
    size_t n_points = expolygon.contour.points.size();
    for (const Domain::Polygon& hole : expolygon.holes) {
        n_points += hole.points.size();
    }

    return n_points;
}

size_t count_points(const Domain::ExPolygons& expolygons)
{
    size_t n_points = 0;
    for (const Domain::ExPolygon& expolygon : expolygons) {
        n_points += expolygon.contour.points.size();
        for (const Domain::Polygon& hole : expolygon.holes) {
            n_points += hole.points.size();
        }
    }

    return n_points;
}

bool contains(const Domain::ExPolygon& expolygon, const Domain::Point& point, const bool border_result)
{
    if (!Polygon::contains(expolygon.contour, point, border_result))
        return false; // Outside the outer contour, not on the contour boundary.

    for (const Domain::Polygon& hole : expolygon.holes) {
        if (Polygon::contains(hole, point, !border_result))
            return false; // Inside a hole, not on the hole boundary.
    }

    return true;
}

bool contains(const Domain::ExPolygons& expolygons, const Domain::Point& point, const bool border_result)
{
    for (const Domain::ExPolygon& expolygon : expolygons) {
        if (ExPolygon::contains(expolygon, point, border_result))
            return true;
    }

    return false;
}

bool contains(const Domain::ExPolygon& expolygon, const Domain::Line& line)
{
    return ExPolygon::contains(expolygon, Domain::Polyline(line.a, line.b));
}

bool contains(const Domain::ExPolygon& expolygon, const Domain::Polyline& polyline)
{
    return Slic3r::diff_pl(polyline, expolygon).empty();
}

bool contains(const Domain::ExPolygon& expolygon, const Polylines& polylines)
{
    return Slic3r::diff_pl(polylines, expolygon).empty();
}

bool overlaps(const Domain::ExPolygon& expolygon, const Domain::ExPolygon &other_expolygon)
{
    if (expolygon.empty() || other_expolygon.empty())
        return false;

    Polylines pl_out = Slic3r::intersection_pl(Slic3r::to_polylines(other_expolygon), expolygon);

    // See unit test SCENARIO("Clipper diff with polyline", "[Clipper]")
    // for in which case the intersection_pl produces any intersection.
    return ! pl_out.empty() ||
        // If *this is completely inside other, then pl_out is empty, but the expolygons overlap. Test for that situation.
        Algorithms::ExPolygon::contains(other_expolygon, expolygon.contour.points.front());
}

Domain::Lines to_lines(const Domain::ExPolygon& expolygon)
{
    Domain::Lines lines;
    lines.reserve(ExPolygon::count_points(expolygon));

    Slic3r::append(lines, Polygon::to_lines(expolygon.contour));
    for (const Domain::Polygon& hole : expolygon.holes) {
        Slic3r::append(lines, Polygon::to_lines(hole));
    }

    return lines;
}

Domain::Lines to_lines(const Domain::ExPolygons& expolygons)
{
    Domain::Lines lines;
    lines.reserve(ExPolygon::count_points(expolygons));

    for (const Domain::ExPolygon& expolygon : expolygons) {
        Slic3r::append(lines, Polygon::to_lines(expolygon.contour));
        for (const Domain::Polygon& hole : expolygon.holes) {
            Slic3r::append(lines, Polygon::to_lines(hole));
        }
    }

    return lines;
}

Domain::ExPolygons simplify(const Domain::ExPolygon& expolygon, const double tolerance)
{
    return Slic3r::union_ex(ExPolygon::simplify_to_polygons(expolygon, tolerance));
}

Domain::Polygons simplify_to_polygons(const Domain::ExPolygon& expolygon, const double tolerance)
{
    Domain::Polygons pp;
    pp.reserve(expolygon.holes.size() + 1);

    // Contour
    {
        Domain::Polygon p = expolygon.contour;
        p.points.push_back(p.points.front());
        p.points = Algorithms::DouglasPeucker::douglas_peucker(p.points, tolerance);
        p.points.pop_back();
        pp.emplace_back(std::move(p));
    }

    // Holes
    for (Domain::Polygon p : expolygon.holes) {
        p.points.push_back(p.points.front());
        p.points = Algorithms::DouglasPeucker::douglas_peucker(p.points, tolerance);
        p.points.pop_back();
        pp.emplace_back(std::move(p));
    }

    return Slic3r::simplify_polygons(pp);
}

} // namespace Slic3r::Biz::Algorithms::ExPolygon
