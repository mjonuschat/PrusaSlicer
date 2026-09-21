///|/ Copyright (c) Prusa Research 2016 - 2021 Vojtěch Bubník @bubnikv
///|/ Copyright (c) SuperSlicer 2019 Remi Durand @supermerill
///|/ Copyright (c) OrcaSlicer 2024 David Eccles @gringer
///|/ Copyright (c) OrcaSlicer 2026 Rodrigo Faselli @RF47
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include <algorithm>
#include <cmath>
#include <vector>
#include <cassert>
#include <cstddef>

#include "Slic3r/Biz/Algorithms/Polygon.hpp"
#include "../ClipperUtils.hpp"
#include "../ShortestPath.hpp"
#include "Fill3DHoneycomb.hpp"
#include "libslic3r/Fill/FillBase.hpp"
#include "libslic3r/Point.hpp"
#include "libslic3r/Polygon.hpp"
#include "libslic3r/libslic3r.h"
#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"

using namespace Slic3r::Biz;

namespace Slic3r {

namespace BB = Biz::Algorithms::BoundingBox;

/*
Creates a contiguous sequence of points at a specified height that make
up a horizontal slice of the edges of a space filling truncated
octahedron tesselation. The octahedrons are oriented so that the
square faces are in the horizontal plane with edges parallel to the X
and Y axes.

Credits: David Eccles (gringer).
*/

template <typename T> int sgn(T val)
{
    return (T(0) < val) - (val < T(0));
}

// Triangular wave function. Has period (gridSize * 2) and amplitude
// (gridSize / 2), with triWave(pos = 0) = 0.
static double triWave(double pos, double gridSize)
{
    float t = float(pos / (gridSize * 2.)) + 0.25f; // convert relative to grid size
    t = t - float(int(t));
    return (1. - std::abs(double(t) * 8. - 4.)) * (gridSize / 4.) + (gridSize / 4.);
}

// Truncated octagonal waveform, with period and offset as per the
// triangular wave function. The Z position adjusts the maximum offset
// [between -(gridSize / 4) and (gridSize / 4)], with period (gridSize * 2)
// and troctWave(Zpos = 0) = 0.
static double troctWave(double pos, double gridSize, double Zpos)
{
    double Zcycle = triWave(Zpos, gridSize);
    double perpOffset = Zcycle / 2;
    double y = triWave(pos, gridSize);
    return (std::abs(y) > std::abs(perpOffset)) ? (sgn(y) * perpOffset) : (y * sgn(perpOffset));
}

// Identify the important points of curve change within a truncated
// octahedron wave (as waveform fraction t):
// 1. Start of wave (always 0.0)
// 2. Transition to upper "horizontal" part
// 3. Transition from upper "horizontal" part
// 4. Transition to lower "horizontal" part
// 5. Transition from lower "horizontal" part
static std::vector<double> getCriticalPoints(double Zpos, double gridSize)
{
    std::vector<double> res = {0.};
    double perpOffset = std::abs(triWave(Zpos, gridSize) / 2.);
    double normalisedOffset = perpOffset / gridSize;
    if (normalisedOffset > 0) {
        res.push_back(gridSize * (0. + normalisedOffset));
        res.push_back(gridSize * (1. - normalisedOffset));
        res.push_back(gridSize * (1. + normalisedOffset));
        res.push_back(gridSize * (2. - normalisedOffset));
    }
    return res;
}

// Generate an array of points that are in the same direction as the
// basic printing line (i.e. Y points for columns, X points for rows).
static std::vector<double> colinearPoints(
    double gridSize, const std::vector<double>& critPoints, const size_t baseLocation,
    size_t gridLength
)
{
    std::vector<double> points;
    points.push_back(double(baseLocation));
    for (double cLoc = double(baseLocation); cLoc < double(gridLength); cLoc += gridSize * 2) {
        for (size_t pi = 0; pi < critPoints.size(); pi++) {
            points.push_back(double(baseLocation) + cLoc + critPoints[pi]);
        }
    }
    points.push_back(double(gridLength));
    return points;
}

// Generate an array of points for the dimension that is perpendicular to
// the basic printing line (i.e. X points for columns, Y points for rows).
static std::vector<double> perpendPoints(
    double Zpos, double gridSize, const std::vector<double>& critPoints, size_t baseLocation,
    size_t gridLength, double offsetBase, double perpDir
)
{
    std::vector<double> points;
    points.push_back(offsetBase);
    for (double cLoc = double(baseLocation); cLoc < double(gridLength); cLoc += gridSize * 2) {
        for (size_t pi = 0; pi < critPoints.size(); pi++) {
            double offset = troctWave(critPoints[pi], gridSize, Zpos);
            points.push_back(offsetBase + (offset * perpDir));
        }
    }
    points.push_back(offsetBase);
    return points;
}

static inline Pointfs zip(const std::vector<double>& x, const std::vector<double>& y)
{
    assert(x.size() == y.size());
    Pointfs out;
    out.reserve(x.size());
    for (size_t i = 0; i < x.size(); ++i)
        out.push_back(Vec2d(x[i], y[i]));
    return out;
}

// Generate a set of curves (array of array of 2d points) that describe a
// horizontal slice of a truncated regular octahedron.
static std::vector<Pointfs> makeActualGrid(
    double Zpos, double gridSize, size_t boundsX, size_t boundsY
)
{
    std::vector<Pointfs> points;
    std::vector<double> critPoints = getCriticalPoints(Zpos, gridSize);
    double zCycle = std::fmod(Zpos + gridSize / 2, gridSize * 2.) / (gridSize * 2.);
    bool printVert = zCycle < 0.5;
    if (printVert) {
        double perpDir = -1;
        for (double x = 0; x <= double(boundsX); x += gridSize, perpDir *= -1) {
            points.push_back(Pointfs());
            Pointfs& newPoints = points.back();
            newPoints = zip(
                perpendPoints(Zpos, gridSize, critPoints, 0, boundsY, x, perpDir),
                colinearPoints(gridSize, critPoints, 0, boundsY)
            );
            if (perpDir == 1)
                std::reverse(newPoints.begin(), newPoints.end());
        }
    } else {
        double perpDir = 1;
        for (double y = gridSize; y <= double(boundsY); y += gridSize, perpDir *= -1) {
            points.push_back(Pointfs());
            Pointfs& newPoints = points.back();
            newPoints = zip(
                colinearPoints(gridSize, critPoints, 0, boundsX),
                perpendPoints(Zpos, gridSize, critPoints, 0, boundsX, y, perpDir)
            );
            if (perpDir == -1)
                std::reverse(newPoints.begin(), newPoints.end());
        }
    }
    return points;
}

// Generate a set of curves (array of array of 2d points) that describe a
// horizontal slice of a truncated regular octahedron with a specified
// grid square size. boundWidth/boundHeight are the bounding box size.
static Polylines makeGrid(double z, double gridSize, double boundWidth, double boundHeight)
{
    std::vector<Pointfs> polylines =
        makeActualGrid(z, gridSize, size_t(boundWidth), size_t(boundHeight));
    Polylines result;
    result.reserve(polylines.size());
    for (auto it_polylines = polylines.begin(); it_polylines != polylines.end(); ++it_polylines) {
        result.push_back(Polyline());
        Polyline& polyline = result.back();
        for (auto it = it_polylines->begin(); it != it_polylines->end(); ++it)
            polyline.points.push_back(Point(coord_t((*it)(0)), coord_t((*it)(1))));
    }
    return result;
}

void Fill3DHoneycomb::_fill_surface_single(
    const FillParams                &params,
    unsigned int                     thickness_layers,
    const std::pair<float, Point>   &direction,
    ExPolygon                        expolygon,
    Polylines                       &polylines_out
)
{
    auto infill_angle = float(this->angle);
    if (std::abs(infill_angle) >= EPSILON)
        expolygon.rotate(-infill_angle);
    BoundingBox bb = Algorithms::Polygon::get_bounding_box(expolygon.contour);

    // With equally-scaled X/Y/Z, the pattern creates a vertically-stretched
    // truncated octahedron, so Z is pre-adjusted by scaling by sqrt(2).
    double zScale = std::sqrt(2.);

    // First guess at the preferred grid size.
    double gridSize = scale_(this->spacing) * ((zScale + 1.) / 2.) / params.density;

    // This density calculation is incorrect for many values > 25%, likely
    // due to quantisation error, so this value is used as a first guess,
    // then the Z scale is adjusted to make the layer patterns consistent.
    // The resultant infill won't be an ideal truncated octahedron, but it
    // looks better than the equivalent quantised version.
    //
    // Use a fixed module height (one layer) rather than the combined layer
    // thickness. With "combine infill every N layers", thickness_layers > 1
    // made the honeycomb pattern inconsistent between Z modules and
    // produced poor bridges.
    double layerHeight = scale_(1.0);
    double layersPerModule = std::floor((gridSize * 2) / (zScale * layerHeight) + 0.05);
    if (params.density > 0.42) { // exact layer pattern for >42% density
        layersPerModule = 2;
        gridSize = scale_(this->spacing) * 1.1 / params.density;
        zScale = (gridSize * 2) / (layersPerModule * layerHeight);
    } else {
        if (layersPerModule < 2)
            layersPerModule = 2;
        zScale = (gridSize * 2) / (layersPerModule * layerHeight);
        gridSize = scale_(this->spacing) * ((zScale + 1.) / 2.) / params.density;
        layersPerModule = std::floor((gridSize * 2) / (zScale * layerHeight) + 0.05);
        if (layersPerModule < 2)
            layersPerModule = 2;
        zScale = (gridSize * 2) / (layersPerModule * layerHeight);
    }

    // Align bounding box to a multiple of our honeycomb grid module (a
    // module is 4*gridSize since one gridSize half-module is growing while
    // the other gridSize half-module is shrinking).
    bb = BB::merge(
        bb, align_to_grid(bb.min, Point(coord_t(gridSize * 4), coord_t(gridSize * 4)))
    );

    Polylines polylines =
        makeGrid(scale_(this->z) * zScale, gridSize, BB::sizes(bb)(0), BB::sizes(bb)(1));

    for (Polyline& pl : polylines)
        pl.translate(bb.min);

    polylines = intersection_pl(polylines, expolygon);

    if (!polylines.empty()) {
        auto infill_start_idx = polylines_out.size(); // only rotate what belongs to us.

        if (params.dont_connect() || polylines.size() <= 1)
            append(polylines_out, chain_polylines(std::move(polylines)));
        else
            this->connect_infill(
                std::move(polylines), expolygon, polylines_out, this->spacing, params
            );

        if (std::abs(infill_angle) >= EPSILON) {
            auto begin = polylines_out.begin() + long(infill_start_idx);
            for (auto it = begin; it != polylines_out.end(); ++it)
                it->rotate(infill_angle);
        }
    }
}

} // namespace Slic3r
