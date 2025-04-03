///|/ Copyright (c) Prusa Research 2016 - 2023 Vojtěch Bubník @bubnikv, Lukáš Matěna @lukasmatena, Lukáš Hejl @hejllukas
///|/ Copyright (c) Slic3r 2013 - 2016 Alessandro Ranellucci @alranel
///|/ Copyright (c) 2015 Maksim Derbasov @ntfshard
///|/ Copyright (c) 2014 Petr Ledvina @ledvinap
///|/
///|/ ported from lib/Slic3r/ExPolygon.pm:
///|/ Copyright (c) Prusa Research 2017 - 2022 Vojtěch Bubník @bubnikv
///|/ Copyright (c) Slic3r 2011 - 2014 Alessandro Ranellucci @alranel
///|/ Copyright (c) 2012 Mark Hindess
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include <ankerl/unordered_dense.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <cstring>

#include "Slic3r/Biz/Algorithms/ExPolygon.hpp"
#include "Slic3r/Biz/Algorithms/DouglasPeucker.hpp"
#include "Slic3r/Biz/Algorithms/Polygon.hpp"
#include "BoundingBox.hpp"
#include "ExPolygon.hpp"
#include "Geometry/MedialAxis.hpp"
#include "Polygon.hpp"
#include "Line.hpp"
#include "ClipperUtils.hpp"
#include "libslic3r/MultiPoint.hpp"
#include "libslic3r/Point.hpp"
#include "libslic3r/Polyline.hpp"
#include "libslic3r/libslic3r.h"

using namespace Slic3r::Biz;

namespace Slic3r {

bool on_boundary(const ExPolygon &expolygon, const Point &point, double eps)
{
    if (::Slic3r::polygon_on_boundary(expolygon.contour, point, eps))
        return true;
    for (const Polygon &hole : expolygon.holes)
        if (::Slic3r::polygon_on_boundary(hole, point, eps))
            return true;
    return false;
}

// Projection of a point onto the polygon.
Point point_projection(const ExPolygon &expolygon, const Point &point)
{
    if (expolygon.holes.empty()) {
        return ::Slic3r::point_projection(expolygon.contour, point);
    } else {
        double dist_min2 = std::numeric_limits<double>::max();
        Point  closest_pt_min;
        for (size_t i = 0; i < expolygon.num_contours(); ++ i) {
            Point closest_pt = ::Slic3r::point_projection(expolygon.contour_or_hole(i), point);
            double d2 = (closest_pt - point).cast<double>().squaredNorm();
            if (d2 < dist_min2) {
                dist_min2      = d2;
                closest_pt_min = closest_pt;
            }
        }
        return closest_pt_min;
    }
}

Points to_points(const ExPolygon &expoly)
{
    return Algorithms::ExPolygon::to_points(expoly);
}

Points to_points(const ExPolygons &src)
{
    return Algorithms::ExPolygon::to_points(src);
}

void medial_axis(const ExPolygon& expolygon, const double min_width, const double max_width, ThickPolylines* polylines)
{
    // init helper object
    Slic3r::Geometry::MedialAxis ma(min_width, max_width, expolygon);
    
    // compute the Voronoi diagram and extract medial axis polylines
    ThickPolylines pp;
    ma.build(&pp);
    
    /*
    Biz::Algorithms::SVG::SVG svg("medial_axis.svg");
    svg.draw(*this);
    svg.draw(pp);
    svg.Close();
    */
    
    /* Find the maximum width returned; we're going to use this for validating and 
       filtering the output segments. */
    double max_w = 0;
    for (ThickPolylines::const_iterator it = pp.begin(); it != pp.end(); ++it)
        max_w = fmaxf(max_w, *std::max_element(it->width.begin(), it->width.end()));
    
    /* Loop through all returned polylines in order to extend their endpoints to the 
       expolygon boundaries */
    bool removed = false;
    for (size_t i = 0; i < pp.size(); ++i) {
        ThickPolyline& polyline = pp[i];
        
        // extend initial and final segments of each polyline if they're actual endpoints
        /* We assign new endpoints to temporary variables because in case of a single-line
           polyline, after we extend the start point it will be caught by the intersection()
           call, so we keep the inner point until we perform the second intersection() as well */
        Point new_front = polyline.points.front();
        Point new_back  = polyline.points.back();
        if (polyline.endpoints.first && !Slic3r::on_boundary(expolygon, new_front, SCALED_EPSILON)) {
            Vec2d p1 = polyline.points.front().cast<double>();
            Vec2d p2 = polyline.points[1].cast<double>();
            // prevent the line from touching on the other side, otherwise intersection() might return that solution
            if (polyline.points.size() == 2)
                p2 = (p1 + p2) * 0.5;
            // Extend the start of the segment.
            p1 -= (p2 - p1).normalized() * max_width;
            if (const std::optional<Point> intersection_pt = Algorithms::Polygon::intersection(expolygon.contour, Line(p1.cast<coord_t>(), p2.cast<coord_t>())); intersection_pt.has_value()) {
                new_front = intersection_pt.value();
            }
        }
        if (polyline.endpoints.second && !Slic3r::on_boundary(expolygon, new_back, SCALED_EPSILON)) {
            Vec2d p1 = (polyline.points.end() - 2)->cast<double>();
            Vec2d p2 = polyline.points.back().cast<double>();
            // prevent the line from touching on the other side, otherwise intersection() might return that solution
            if (polyline.points.size() == 2)
                p1 = (p1 + p2) * 0.5;
            // Extend the start of the segment.
            p2 += (p2 - p1).normalized() * max_width;
            if (const std::optional<Point> intersection_pt = Algorithms::Polygon::intersection(expolygon.contour, Line(p1.cast<coord_t>(), p2.cast<coord_t>())); intersection_pt.has_value()) {
                new_back = intersection_pt.value();
            }
        }
        polyline.points.front() = new_front;
        polyline.points.back()  = new_back;
        
        /*  remove too short polylines
            (we can't do this check before endpoints extension and clipping because we don't
            know how long will the endpoints be extended since it depends on polygon thickness
            which is variable - extension will be <= max_width/2 on each side)  */
        if ((polyline.endpoints.first || polyline.endpoints.second)
            && polyline.length() < max_w*2) {
            pp.erase(pp.begin() + i);
            --i;
            removed = true;
            continue;
        }
    }
    
    /*  If we removed any short polylines we now try to connect consecutive polylines
        in order to allow loop detection. Note that this algorithm is greedier than 
        MedialAxis::process_edge_neighbors() as it will connect random pairs of 
        polylines even when more than two start from the same point. This has no 
        drawbacks since we optimize later using nearest-neighbor which would do the 
        same, but should we use a more sophisticated optimization algorithm we should
        not connect polylines when more than two meet.  */
    if (removed) {
        for (size_t i = 0; i < pp.size(); ++i) {
            ThickPolyline& polyline = pp[i];
            if (polyline.endpoints.first && polyline.endpoints.second) continue; // optimization
            
            // find another polyline starting here
            for (size_t j = i+1; j < pp.size(); ++j) {
                ThickPolyline& other = pp[j];
                if (polyline.last_point() == other.last_point()) {
                    other.reverse();
                } else if (polyline.first_point() == other.last_point()) {
                    polyline.reverse();
                    other.reverse();
                } else if (polyline.first_point() == other.first_point()) {
                    polyline.reverse();
                } else if (polyline.last_point() != other.first_point()) {
                    continue;
                }
                
                polyline.points.insert(polyline.points.end(), other.points.begin() + 1, other.points.end());
                polyline.width.insert(polyline.width.end(), other.width.begin(), other.width.end());
                polyline.endpoints.second = other.endpoints.second;
                assert(polyline.width.size() == polyline.points.size()*2 - 2);
                
                pp.erase(pp.begin() + j);
                j = i;  // restart search from i+1
            }
        }
    }
    
    polylines->insert(polylines->end(), pp.begin(), pp.end());
}

void medial_axis(const ExPolygon& expolygon, const double min_width, const double max_width, Polylines* polylines)
{
    ThickPolylines tp;
    Slic3r::medial_axis(expolygon, min_width, max_width, &tp);
    polylines->reserve(polylines->size() + tp.size());
    for (auto &pl : tp)
        polylines->emplace_back(pl.points);
}

Polylines medial_axis(const ExPolygon& expolygon, const double min_width, const double max_width)
{
    Polylines out;
    Slic3r::medial_axis(expolygon, min_width, max_width, &out);
    return out;
}

// Do expolygons match? If they match, they must have the same topology,
// however their contours may be rotated.
bool expolygons_match(const ExPolygon &l, const ExPolygon &r)
{
    if (l.holes.size() != r.holes.size() || ! polygons_match(l.contour, r.contour))
        return false;
    for (size_t hole_idx = 0; hole_idx < l.holes.size(); ++ hole_idx)
        if (! polygons_match(l.holes[hole_idx], r.holes[hole_idx]))
            return false;
    return true;
}

BoundingBox get_extents(const ExPolygon &expolygon)
{
    const auto bb{Algorithms::ExPolygon::get_extents(expolygon)};
    BoundingBox result{bb.min, bb.max};
    result.defined = bb.defined;
    return result;
}

BoundingBox get_extents(const ExPolygons &expolygons)
{
    const auto bb{Algorithms::ExPolygon::get_extents(expolygons)};
    BoundingBox result{bb.min, bb.max};
    result.defined = bb.defined;
    return result;
}

BoundingBox get_extents_rotated(const ExPolygon &expolygon, double angle)
{
    return get_extents_rotated(expolygon.contour, angle);
}

BoundingBox get_extents_rotated(const ExPolygons &expolygons, double angle)
{
    BoundingBox bbox;
    if (! expolygons.empty()) {
        bbox = get_extents_rotated(expolygons.front().contour, angle);
        for (size_t i = 1; i < expolygons.size(); ++ i)
            bbox.merge(get_extents_rotated(expolygons[i].contour, angle));
    }
    return bbox;
}

extern std::vector<BoundingBox> get_extents_vector(const ExPolygons &polygons)
{
    std::vector<BoundingBox> out;
    out.reserve(polygons.size());
    for (ExPolygons::const_iterator it = polygons.begin(); it != polygons.end(); ++ it)
        out.push_back(get_extents(*it));
    return out;
}

bool has_duplicate_points(const ExPolygon &expoly)
{
#if 1
    // Check globally.
    size_t cnt = expoly.contour.points.size();
    for (const Polygon &hole : expoly.holes)
        cnt += hole.points.size();
    Points allpts;
    allpts.reserve(cnt);
    allpts.insert(allpts.begin(), expoly.contour.points.begin(), expoly.contour.points.end());
    for (const Polygon &hole : expoly.holes)
        allpts.insert(allpts.end(), hole.points.begin(), hole.points.end());
    return has_duplicate_points(std::move(allpts));
#else
    // Check per contour.
    if (has_duplicate_points(expoly.contour))
        return true;
    for (const Polygon &hole : expoly.holes)
        if (has_duplicate_points(hole))
            return true;
    return false;
#endif
}

bool has_duplicate_points(const ExPolygons &expolys)
{
#if 1
    // Check globally.
#if 0
    // Detect duplicates by sorting with quicksort. It is quite fast, but ankerl::unordered_dense is around 1/4 faster.
    Points allpts;
    allpts.reserve(count_points(expolys));
    for (const ExPolygon &expoly : expolys) {
        allpts.insert(allpts.begin(), expoly.contour.points.begin(), expoly.contour.points.end());
        for (const Polygon &hole : expoly.holes)
            allpts.insert(allpts.end(), hole.points.begin(), hole.points.end());
    }
    return has_duplicate_points(std::move(allpts));
#else
    // Detect duplicates by inserting into an ankerl::unordered_dense hash set, which is is around 1/4 faster than qsort.
    struct PointHash {
        uint64_t operator()(const Point &p) const noexcept {
            uint64_t h;
            static_assert(sizeof(h) == sizeof(p));
            memcpy(&h, &p, sizeof(p));
            return ankerl::unordered_dense::detail::wyhash::hash(h);
        }
    };
    ankerl::unordered_dense::set<Point, PointHash> allpts;
    allpts.reserve(count_points(expolys));
    for (const ExPolygon &expoly : expolys)
        for (size_t icontour = 0; icontour < expoly.num_contours(); ++ icontour)
            for (const Point &pt : expoly.contour_or_hole(icontour).points)
                if (! allpts.insert(pt).second)
                    // Duplicate point was discovered.
                    return true;
    return false;
#endif
#else
    // Check per contour.
    for (const ExPolygon &expoly : expolys)
        if (has_duplicate_points(expoly))
            return true;
    return false;
#endif
}

bool remove_same_neighbor(ExPolygons &expolygons)
{
    if (expolygons.empty())
        return false;
    bool remove_from_holes   = false;
    bool remove_from_contour = false;
    for (ExPolygon &expoly : expolygons) {
        remove_from_contour |= remove_same_neighbor(expoly.contour);
        remove_from_holes |= remove_same_neighbor(expoly.holes);
    }
    // Removing of expolygons without contour
    if (remove_from_contour)
        expolygons.erase(std::remove_if(expolygons.begin(), expolygons.end(),
                                        [](const ExPolygon &p) { return p.contour.points.size() <= 2; }),
                         expolygons.end());
    return remove_from_holes || remove_from_contour;
}

bool remove_sticks(ExPolygon &poly)
{
    return remove_sticks(poly.contour) || remove_sticks(poly.holes);
}

bool remove_small_and_small_holes(ExPolygons &expolygons, double min_area)
{
    bool   modified = false;
    size_t free_idx = 0;
    for (size_t expoly_idx = 0; expoly_idx < expolygons.size(); ++expoly_idx) {
        if (std::abs(expolygons[expoly_idx].area()) >= min_area) {
            // Expolygon is big enough, so also check all its holes
            modified |= remove_small(expolygons[expoly_idx].holes, min_area);
            if (free_idx < expoly_idx) {
                std::swap(expolygons[expoly_idx].contour, expolygons[free_idx].contour);
                std::swap(expolygons[expoly_idx].holes, expolygons[free_idx].holes);
            }
            ++free_idx;
        } else
            modified = true;
    }
    if (free_idx < expolygons.size())
        expolygons.erase(expolygons.begin() + free_idx, expolygons.end());
    return modified;
}

void keep_largest_contour_only(ExPolygons &polygons)
{
	if (polygons.size() > 1) {
	    double     max_area = 0.;
	    ExPolygon* max_area_polygon = nullptr;
	    for (ExPolygon& p : polygons) {
	        double a = p.contour.area();
	        if (a > max_area) {
	            max_area         = a;
	            max_area_polygon = &p;
	        }
	    }
	    assert(max_area_polygon != nullptr);
	    ExPolygon p(std::move(*max_area_polygon));
	    polygons.clear();
	    polygons.emplace_back(std::move(p));
	}
}

} // namespace Slic3r
