///|/ Copyright (c) BambuStudio 2023 Bambu Lab
///|/ Copyright (c) OrcaSlicer 2024 SoftFever @SoftFever
///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include <algorithm>
#include <cmath>

#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"
#include "Slic3r/Biz/Algorithms/Polygon.hpp"
#include "libslic3r/ClipperUtils.hpp"
#include "libslic3r/Fill/boss/crosshatch/FillCrossHatch.hpp"
#include "libslic3r/Polygon.hpp"
#include "libslic3r/ShortestPath.hpp"

using namespace Slic3r::Biz;

namespace Slic3r {

// CrossHatch infill alternates line direction by 90 degrees every few layers to improve
// adhesion, with transform layers between direction shifts for better line cohesion.
// The transform technique is inspired by David Eccles' improved 3D honeycomb, adapted
// here with a more flexible implementation.
//
// graph credits: David Eccles (gringer), with a different point layout:
/*    o---o
 *   /     \
 *  /       \
 *           \       /
 *            \     /
 *             o---o
 *   p1   p2  p3   p4
 */

static void transpose_if_vertical(int direction, Polylines &polylines)
{
    if (direction >= 0) return;
    for (Polyline &poly : polylines) {
        for (Point &p : poly) { std::swap(p.x(), p.y()); }
    }
}

static Pointfs generate_one_cycle(double progress, double period)
{
    Pointfs out;
    double  offset = progress * 1. / 8. * period;
    out.reserve(4);
    out.push_back(Vec2d(0.25 * period - offset, offset));
    out.push_back(Vec2d(0.25 * period + offset, offset));
    out.push_back(Vec2d(0.75 * period - offset, -offset));
    out.push_back(Vec2d(0.75 * period + offset, -offset));
    return out;
}

static Polylines generate_transform_pattern(double inprogress, int direction, double ingrid_size, double inwidth, double inheight)
{
    double  width     = inwidth;
    double  height    = inheight;
    double  grid_size = ingrid_size * 2;
    double    progress  = inprogress;
    Polylines out_polylines;

    Pointfs one_cycle_points = generate_one_cycle(progress, grid_size);

    Polyline one_cycle;
    one_cycle.points.reserve(one_cycle_points.size());
    for (size_t i = 0; i < one_cycle_points.size(); i++)
        one_cycle.points.push_back(Point(coord_t(one_cycle_points[i].x()), coord_t(one_cycle_points[i].y())));

    if (direction < 0) {
        width  = height;
        height = inwidth;
    }

    Polylines odd_polylines;
    Polyline  odd_poly;
    int       num_of_cycle = width / grid_size + 2;
    odd_poly.points.reserve(num_of_cycle * one_cycle.size());

    for (size_t i = 0; i < num_of_cycle; i++) {
        Polyline odd_points;
        odd_points = Polyline(one_cycle);
        odd_points.translate(Point(i * grid_size, 0.0));
        odd_poly.points.insert(odd_poly.points.end(), odd_points.begin(), odd_points.end());
    }

    int num_of_lines = height / grid_size + 2;
    odd_polylines.reserve(num_of_lines * odd_poly.size());
    for (size_t i = 0; i < num_of_lines; i++) {
        Polyline poly = odd_poly;
        poly.translate(Point(0.0, grid_size * i));
        odd_polylines.push_back(poly);
    }
    out_polylines.insert(out_polylines.end(), odd_polylines.begin(), odd_polylines.end());

    Polylines even_polylines;
    even_polylines.reserve(odd_polylines.size());
    for (size_t i = 0; i < odd_polylines.size(); i++) {
        Polyline even = odd_poly;
        even.translate(Point(-0.5 * grid_size, (i + 0.5) * grid_size));
        even_polylines.push_back(even);
    }

    out_polylines.insert(out_polylines.end(), even_polylines.begin(), even_polylines.end());

    transpose_if_vertical(direction, out_polylines);

    return out_polylines;
}

static Polylines generate_repeat_pattern(int direction, double grid_size, double inwidth, double inheight)
{
    double  width  = inwidth;
    double  height = inheight;
    Polylines out_polylines;

    if (direction < 0) {
        width  = height;
        height = inwidth;
    }

    int num_of_lines = height / grid_size + 1;
    out_polylines.reserve(num_of_lines);

    for (int i = 0; i < num_of_lines; i++) {
        Polyline poly;
        poly.points.reserve(2);
        poly.append(Point(double(0), double(grid_size * i)));
        poly.append(Point(width, double(grid_size * i)));
        out_polylines.push_back(poly);
    }

    transpose_if_vertical(direction, out_polylines);

    return out_polylines;
}

static double low_density_repeat_ratio(double density)
{
    if (density >= 0.3) return 1.0;
    return std::clamp(1.0 - std::exp(-5 * density), 0.2, 1.0);
}

// Builds the polylines that overlap the bounding box for one print layer. `repeat_ratio`
// sets the ratio between the height of a repeat (straight-line) pattern band and the grid
// size; it shrinks at low density so thin, sparse infill still gets enough transform
// bands to stay crosshatched instead of collapsing into plain parallel lines.
static Polylines generate_infill_layers(double z_height, double repeat_ratio, double grid_size, double width, double height)
{
    Polylines result;
    double  trans_layer_size  = grid_size * 0.4;          // upper band: direction-transform layer
    double  repeat_layer_size = grid_size * repeat_ratio; // lower band: straight repeat layer
    z_height                    += repeat_layer_size / 2 + trans_layer_size;   // offset to improve first few layer strength and reduce warping risk
    double  period            = trans_layer_size + repeat_layer_size;
    double  remains           = z_height - std::floor(z_height / period) * period;
    double  trans_z           = remains - repeat_layer_size; // put repeat layer first

    int phase     = fmod(z_height, period * 2) - (period - 1); // epsilon padding avoids flipping direction right at the band boundary
    int direction = phase <= 0 ? -1 : 1;

    if (trans_z < 0) {
        // this z lands in the repeat band
        result = generate_repeat_pattern(direction, grid_size, width, height);
    } else {
        // this z lands in the transform band: split progress into a forward half and a
        // backward half with the opposite direction, so consecutive transform layers
        // mesh into each other instead of all shifting the same way
        double progress = fmod(trans_z, trans_layer_size) / trans_layer_size;

        if (progress < 0.5)
            result = generate_transform_pattern((progress + 0.1) * 2, direction, grid_size, width, height); // increase overlapping
        else
            result = generate_transform_pattern((1.1 - progress) * 2, -1 * direction, grid_size, width, height);
    }

    return result;
}

void FillCrossHatch::_fill_surface_single(
    const FillParams &params, unsigned int thickness_layers, const std::pair<float, Point> &direction, ExPolygon expolygon, Polylines &polylines_out)
{
    auto infill_angle = float(this->angle);
    if (std::abs(infill_angle) >= EPSILON) expolygon.rotate(-infill_angle);

    BoundingBox bb = Algorithms::Polygon::get_bounding_box(expolygon.contour);

    coord_t line_spacing = coord_t(scale_(this->spacing) / params.density);

    if (params.density < 0.999) line_spacing *= 1.08;

    bb = Algorithms::BoundingBox::merge(bb, align_to_grid(bb.min, Point(line_spacing * 4, line_spacing * 4)));

    double repeat_ratio = low_density_repeat_ratio(params.density);

    Polylines polylines = generate_infill_layers(scale_(this->z), repeat_ratio, line_spacing,
                                                  Algorithms::BoundingBox::sizes(bb)(0), Algorithms::BoundingBox::sizes(bb)(1));

    for (Polyline &pl : polylines) { pl.translate(bb.min); }

    polylines = intersection_pl(polylines, expolygon);

    if (!polylines.empty()) {
        const double min_thin_wall_preserving_length = scale_(0.8 * this->spacing);
        polylines.erase(std::remove_if(polylines.begin(), polylines.end(), [min_thin_wall_preserving_length](const Polyline &pl)
            { return pl.length() < min_thin_wall_preserving_length; }), polylines.end());
    }

    if (!polylines.empty()) {
        int infill_start_idx = polylines_out.size();
        if (params.dont_connect() || polylines.size() <= 1)
            append(polylines_out, chain_polylines(std::move(polylines)));
        else
            this->connect_infill(std::move(polylines), expolygon, polylines_out, this->spacing, params);

        if (std::abs(infill_angle) >= EPSILON) {
            for (auto it = polylines_out.begin() + infill_start_idx; it != polylines_out.end(); ++it) it->rotate(infill_angle);
        }
    }
}

} // namespace Slic3r

#include "boss/features/crosshatch/CrossHatchFeature.hpp"

namespace Slic3r::Boss {

std::unique_ptr<Fill> CrossHatchFeature::create_fill()
{
    return std::make_unique<Slic3r::FillCrossHatch>();
}

bool CrossHatchFeature::use_bridge_flow()
{
    return false;
}

} // namespace Slic3r::Boss
