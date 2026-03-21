///|/ Copyright (c) Prusa Research 2016 - 2022 Vojtěch Bubník @bubnikv, Lukáš Matěna @lukasmatena
///|/
///|/ ported from lib/Slic3r/Fill/Concentric.pm:
///|/ Copyright (c) Prusa Research 2016 Vojtěch Bubník @bubnikv
///|/ Copyright (c) Slic3r 2011 - 2015 Alessandro Ranellucci @alranel
///|/ Copyright (c) 2012 Mark Hindess
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include <array>
#include <cmath>
#include <cstdint>

#include "../ClipperUtils.hpp"
#include "../ShortestPath.hpp"
#include "FillPlanePath.hpp"
#include "libslic3r/BoundingBox.hpp"
#include "libslic3r/Fill/FillBase.hpp"

namespace Slic3r {

class InfillPolylineClipper : public FillPlanePath::InfillPolylineOutput {
public:
    InfillPolylineClipper(const BoundingBox bbox, const double scale_out) : FillPlanePath::InfillPolylineOutput(scale_out), m_bbox(bbox) {}

    void            add_point(const Vec2d &pt);
    Points&&        result() { return std::move(m_out); }
    bool            clips() const override { return true; }

private:
    enum class Side {
        Left   = 1,
        Right  = 2,
        Top    = 4,
        Bottom = 8
    };

    int sides(const Point &p) const {
        return int(p.x() < m_bbox.min.x()) * int(Side::Left) +
               int(p.x() > m_bbox.max.x()) * int(Side::Right) +
               int(p.y() < m_bbox.min.y()) * int(Side::Bottom) +
               int(p.y() > m_bbox.max.y()) * int(Side::Top);
    };

    // Bounding box to clip the polyline with.
    BoundingBox m_bbox;

    // Classification of the two last points processed.
    int         m_sides_prev;
    int         m_sides_this;
};

void InfillPolylineClipper::add_point(const Vec2d &fpt)
{
    const Point pt{ this->scaled(fpt) };

    if (m_out.size() < 2) {
        // Collect the two first points and their status.
        (m_out.empty() ? m_sides_prev : m_sides_this) = sides(pt);
        m_out.emplace_back(pt);
    } else {
        // Classify the last inserted point, possibly remove it.
        int sides_next = sides(pt);
        if (// This point is inside. Take it.
            m_sides_this == 0 ||
            // Either this point is outside and previous or next is inside, or
            // the edge possibly cuts corner of the bounding box.
            (m_sides_prev & m_sides_this & sides_next) == 0) {
            // Keep the last point.
            m_sides_prev = m_sides_this;
        } else {
            // All the three points (this, prev, next) are outside at the same side.
            // Ignore the last point.
            m_out.pop_back();
        }
        // And save the current point.
        m_out.emplace_back(pt);
        m_sides_this = sides_next;
    }
}

void FillPlanePath::_fill_surface_single(
    const FillParams                &params, 
    unsigned int                     thickness_layers,
    const std::pair<float, Point>   &direction, 
    ExPolygon                        expolygon,
    Polylines                       &polylines_out)
{
    expolygon.rotate(-direction.first);

    //FIXME Vojtech: We are not sure whether the user expects the fill patterns on visible surfaces to be aligned across all the islands of a single layer.
    // One may align for this->centered() to align the patterns for Archimedean Chords and Octagram Spiral patterns.
    const bool align = params.density < 0.995;

    BoundingBox snug_bounding_box = get_extents(expolygon).inflated(SCALED_EPSILON);

    // Rotated bounding box of the area to fill in with the pattern.
    BoundingBox bounding_box = align ?
        // Sparse infill needs to be aligned across layers. Align infill across layers using the object's bounding box.
        this->bounding_box.rotated(-direction.first) :
        // Solid infill does not need to be aligned across layers, generate the infill pattern
        // around the clipping expolygon only.
        snug_bounding_box;

    Point shift = this->centered() ? 
        bounding_box.center() :
        bounding_box.min;
    expolygon.translate(-shift.x(), -shift.y());
    bounding_box.translate(-shift.x(), -shift.y());

    Polyline polyline;
    {
        auto distance_between_lines = scaled<double>(this->spacing) / params.density;
        auto min_x = coord_t(ceil(coordf_t(bounding_box.min.x()) / distance_between_lines));
        auto min_y = coord_t(ceil(coordf_t(bounding_box.min.y()) / distance_between_lines));
        auto max_x = coord_t(ceil(coordf_t(bounding_box.max.x()) / distance_between_lines));
        auto max_y = coord_t(ceil(coordf_t(bounding_box.max.y()) / distance_between_lines));
        auto resolution = scaled<double>(params.resolution) / distance_between_lines;
        if (align) {
            // Filling in a bounding box over the whole object, clip generated polyline against the snug bounding box.
            snug_bounding_box.translate(-shift.x(), -shift.y());
            InfillPolylineClipper output(snug_bounding_box, distance_between_lines);
            this->generate(min_x, min_y, max_x, max_y, resolution, output);
            polyline.points = std::move(output.result());
        } else {
            // Filling in a snug bounding box, no need to clip.
            InfillPolylineOutput output(distance_between_lines);
            this->generate(min_x, min_y, max_x, max_y, resolution, output);
            polyline.points = std::move(output.result());
        }
    }

    if (polyline.size() >= 2) {
        Polylines polylines = intersection_pl(polyline, expolygon);
        Polylines chained;
        if (params.dont_connect() || params.density > 0.5 || polylines.size() <= 1)
            chained = chain_polylines(std::move(polylines));
        else
            connect_infill(std::move(polylines), expolygon, chained, this->spacing, params);
        // paths must be repositioned and rotated back
        for (Polyline &pl : chained) {
            pl.translate(shift.x(), shift.y());
            pl.rotate(direction.first);
        }
        append(polylines_out, std::move(chained));
    }
}

// Follow an Archimedean spiral, in polar coordinates: r=a+b\theta
template<typename Output>
static void generate_archimedean_chords(coord_t min_x, coord_t min_y, coord_t max_x, coord_t max_y, const double resolution, Output &output)
{
    // Radius to achieve.
    coordf_t rmax = std::sqrt(coordf_t(max_x)*coordf_t(max_x)+coordf_t(max_y)*coordf_t(max_y)) * std::sqrt(2.) + 1.5;
    // Now unwind the spiral.
    coordf_t a = 1.;
    coordf_t b = 1./(2.*M_PI);
    coordf_t theta = 0.;
    coordf_t r = 1;
    Pointfs out;
    //FIXME Vojtech: If used as a solid infill, there is a gap left at the center.
    output.add_point({ 0, 0 });
    output.add_point({ 1, 0 });
    while (r < rmax) {
        // Discretization angle to achieve a discretization error lower than resolution.
        theta += 2. * acos(1. - resolution / r);
        r = a + b * theta;
        output.add_point({ r * cos(theta), r * sin(theta) });
    }
}

void FillArchimedeanChords::generate(coord_t min_x, coord_t min_y, coord_t max_x, coord_t max_y, const double resolution, InfillPolylineOutput &output)
{
    if (output.clips())
        generate_archimedean_chords(min_x, min_y, max_x, max_y, resolution, static_cast<InfillPolylineClipper&>(output));
    else
        generate_archimedean_chords(min_x, min_y, max_x, max_y, resolution, output);
}

// Adapted from 
// http://cpansearch.perl.org/src/KRYDE/Math-PlanePath-122/lib/Math/PlanePath/HilbertCurve.pm
//
// state=0    3--2   plain
//               |
//            0--1
//
// state=4    1--2  transpose
//            |  |
//            0  3
//
// state=8
//
// state=12   3  0  rot180 + transpose
//            |  |
//            2--1
//
static inline Point hilbert_n_to_xy(const size_t n)
{
    static constexpr const int next_state[16] { 4,0,0,12, 0,4,4,8, 12,8,8,4, 8,12,12,0 };
    static constexpr const int digit_to_x[16] { 0,1,1,0, 0,0,1,1, 1,0,0,1, 1,1,0,0 };
    static constexpr const int digit_to_y[16] { 0,0,1,1, 0,1,1,0, 1,1,0,0, 1,0,0,1 };

    // Number of 2 bit digits.
    size_t ndigits = 0;
    {
        size_t nc = n;
        while(nc > 0) {
            nc >>= 2;
            ++ ndigits;
        }
    }
    int state = (ndigits & 1) ? 4 : 0;
    coord_t x = 0;
    coord_t y = 0;
    for (int i = (int)ndigits - 1; i >= 0; -- i) {
        int digit = (n >> (i * 2)) & 3;
        state += digit;
        x |= digit_to_x[state] << i;
        y |= digit_to_y[state] << i;
        state = next_state[state];
    }
    return Point(x, y);
}

template<typename Output>
static void generate_hilbert_curve(coord_t min_x, coord_t min_y, coord_t max_x, coord_t max_y, Output &output)
{
    // Minimum power of two square to fit the domain.
    size_t sz = 2;
    size_t pw = 1;
    {
        size_t sz0 = std::max(max_x + 1 - min_x, max_y + 1 - min_y);
        while (sz < sz0) {
            sz = sz << 1;
            ++ pw;
        }
    }

    size_t sz2 = sz * sz;
    output.reserve(sz2);
    for (size_t i = 0; i < sz2; ++ i) {
        Point p = hilbert_n_to_xy(i);
        output.add_point({ p.x() + min_x, p.y() + min_y });
    }
}

void FillHilbertCurve::generate(coord_t min_x, coord_t min_y, coord_t max_x, coord_t max_y, const double /* resolution */, InfillPolylineOutput &output)
{
    if (output.clips())
        generate_hilbert_curve(min_x, min_y, max_x, max_y, static_cast<InfillPolylineClipper&>(output));
    else
        generate_hilbert_curve(min_x, min_y, max_x, max_y, output);
}

template<typename Output>
static void generate_octagram_spiral(coord_t min_x, coord_t min_y, coord_t max_x, coord_t max_y, Output &output)
{
    // Radius to achieve.
    coordf_t rmax = std::sqrt(coordf_t(max_x)*coordf_t(max_x)+coordf_t(max_y)*coordf_t(max_y)) * std::sqrt(2.) + 1.5;
    // Now unwind the spiral.
    coordf_t r = 0;
    coordf_t r_inc = sqrt(2.);
    output.add_point({ 0., 0. });
    while (r < rmax) {
        r += r_inc;
        coordf_t rx = r / sqrt(2.);
        coordf_t r2 = r + rx;
        output.add_point({ r,   0. });
        output.add_point({ r2,  rx });
        output.add_point({ rx,  rx });
        output.add_point({ rx,  r2 });
        output.add_point({ 0.,  r  });
        output.add_point({-rx,  r2 });
        output.add_point({-rx,  rx });
        output.add_point({-r2,  rx });
        output.add_point({- r,  0. });
        output.add_point({-r2, -rx });
        output.add_point({-rx, -rx });
        output.add_point({-rx, -r2 });
        output.add_point({ 0., -r  });
        output.add_point({ rx, -r2 });
        output.add_point({ rx, -rx });
        output.add_point({ r2+r_inc, -rx });
    }
}

void FillOctagramSpiral::generate(coord_t min_x, coord_t min_y, coord_t max_x, coord_t max_y, const double /* resolution */, InfillPolylineOutput &output)
{
    if (output.clips())
        generate_octagram_spiral(min_x, min_y, max_x, max_y, static_cast<InfillPolylineClipper&>(output));
    else
        generate_octagram_spiral(min_x, min_y, max_x, max_y, output);
}

// Gosper curve (Flowsnake / Peano-Gosper curve)
// A space-filling fractal curve based on hexagonal tiling.
// L-system: A -> A-B--B+A++AA+B-  B -> +A-BB--B-A++A+B  angle = 60°
// Each level replaces 1 segment with 7 sub-segments (sqrt(7) scaling).
//
// Uses recursive walk with precomputed direction tables for the 6 hex directions.
// Coordinates are in floating-point (hexagonal grid, not axis-aligned integer grid).

// Precomputed cos/sin for the 6 hex directions (multiples of 60°)
static constexpr double gosper_cos[6] = { 1.0, 0.5, -0.5, -1.0, -0.5, 0.5 };
static constexpr double gosper_sin[6] = { 0.0, 0.8660254037844386, 0.8660254037844386, 0.0, -0.8660254037844386, -0.8660254037844386 };

// Fast direction update: dir_add[dir][turn+2] gives the new direction.
// Avoids modulo operations in the hot loop. Turn range is [-2, +2].
static constexpr int8_t gosper_dir_add[6][5] = {
    // turn: -2  -1   0  +1  +2
    {  4,  5,  0,  1,  2 },  // dir 0
    {  5,  0,  1,  2,  3 },  // dir 1
    {  0,  1,  2,  3,  4 },  // dir 2
    {  1,  2,  3,  4,  5 },  // dir 3
    {  2,  3,  4,  5,  0 },  // dir 4
    {  3,  4,  5,  0,  1 },  // dir 5
};

// Sub-segment tables for the Gosper curve L-system rules.
// Each entry: { is_a (true=A, false=B), turn (in 60° increments, applied BEFORE this sub-segment) }
// A -> A-B--B+A++AA+B-
// Turns between segments:  (0) A, (-1) B, (-2) B, (+1) A, (+2) A, (0) A, (+1) B, (-1 trailing)
// B -> +A-BB--B-A++A+B
// Turns: (+1 leading) A, (-1) B, (0) B, (-2) B, (-1) A, (+2) A, (+1) B
struct GosperSub { bool is_a; int turn; };

static constexpr GosperSub gosper_a_subs[7] = {
    { true,   0 },  // A (no turn before first)
    { false, -1 },  // -B
    { false, -2 },  // --B
    { true,   1 },  // +A
    { true,   2 },  // ++A
    { true,   0 },  // A (no turn, consecutive)
    { false,  1 },  // +B
};
static constexpr int gosper_a_trailing_turn = -1;  // trailing -

static constexpr GosperSub gosper_b_subs[7] = {
    { true,   1 },  // +A (leading +)
    { false, -1 },  // -B
    { false,  0 },  // B (no turn, consecutive BB)
    { false, -2 },  // --B
    { true,  -1 },  // -A
    { true,   2 },  // ++A
    { false,  1 },  // +B
};
static constexpr int gosper_b_trailing_turn = 0;  // no trailing turn

// Iterative Gosper curve walk using an explicit stack.
// Eliminates recursive function call overhead (~823K calls at level 7).
// The stack is at most 8 frames deep (64 bytes), fitting in a single cache line.
struct GosperFrame { int8_t level; bool is_a; int8_t sub_idx; };

template<typename Output>
static void gosper_walk_iterative(int level, double &x, double &y, Output &output)
{
    static constexpr int max_level = 7;
    std::array<GosperFrame, max_level + 1> stack;
    assert(level <= max_level);
    int sp = 0;
    int dir = 0;
    stack[0] = { int8_t(level), true, 0 };

    while (sp >= 0) {
        GosperFrame &f = stack[sp];

        if (f.level == 0) {
            // Base case: emit one segment in current direction
            x += gosper_cos[dir];
            y += gosper_sin[dir];
            output.add_point({ x, y });
            --sp;
            if (sp >= 0)
                ++stack[sp].sub_idx;
            continue;
        }

        if (f.sub_idx >= 7) {
            // All 7 sub-segments done — apply trailing turn and pop
            int trailing = f.is_a ? gosper_a_trailing_turn : gosper_b_trailing_turn;
            if (trailing != 0)
                dir = gosper_dir_add[dir][trailing + 2];
            --sp;
            if (sp >= 0)
                ++stack[sp].sub_idx;
            continue;
        }

        // Push next sub-segment onto the stack
        const GosperSub &sub = (f.is_a ? gosper_a_subs : gosper_b_subs)[f.sub_idx];
        dir = gosper_dir_add[dir][sub.turn + 2];
        ++sp;
        stack[sp] = { int8_t(f.level - 1), sub.is_a, 0 };
    }
}

// Precomputed bounding box centers for each Gosper curve level.
// Computed once on first use, cached for all subsequent calls.
struct GosperBBox { double cx, cy; };

static GosperBBox gosper_get_bbox(int level)
{
    // Thread-safe static local cache (C++11 guarantee)
    static std::array<GosperBBox, 8> cache = []() {
        std::array<GosperBBox, 8> result{};
        for (int lv = 1; lv <= 7; ++lv) {
            double x = 0.0, y = 0.0;
            double bmin_x = 0.0, bmax_x = 0.0, bmin_y = 0.0, bmax_y = 0.0;
            struct Tracker {
                double &bmin_x, &bmax_x, &bmin_y, &bmax_y;
                void reserve(size_t) {}
                void add_point(const Vec2d &pt) {
                    bmin_x = std::min(bmin_x, pt.x());
                    bmax_x = std::max(bmax_x, pt.x());
                    bmin_y = std::min(bmin_y, pt.y());
                    bmax_y = std::max(bmax_y, pt.y());
                }
                bool clips() const { return false; }
            } tracker{bmin_x, bmax_x, bmin_y, bmax_y};
            gosper_walk_iterative(lv, x, y, tracker);
            result[lv] = { (bmin_x + bmax_x) * 0.5, (bmin_y + bmax_y) * 0.5 };
        }
        return result;
    }();
    return cache[level];
}

template<typename Output>
static void generate_flowsnake(coord_t min_x, coord_t min_y, coord_t max_x, coord_t max_y, Output &output)
{
    static constexpr double sqrt7 = 2.6457513110645906;
    double extent = std::max(max_x + 1 - min_x, max_y + 1 - min_y);

    // Find the minimum level where the Gosper curve covers the bounding box.
    int level = 0;
    double coverage = 1.0;
    while (coverage < extent * 2.0 && level < 7) {
        coverage *= sqrt7;
        ++level;
    }
    if (level < 1) level = 1;

    // Cached bounding box center — computed once per level, reused across all calls
    GosperBBox bbox = gosper_get_bbox(level);

    // Translation to center the curve on the fill region
    double fill_cx = (min_x + max_x) * 0.5;
    double fill_cy = (min_y + max_y) * 0.5;

    // Total points = 7^level + 1
    size_t total_segments = 1;
    for (int i = 0; i < level; ++i)
        total_segments *= 7;
    output.reserve(total_segments + 1);

    // Generate directly to output with translation applied
    double x = fill_cx - bbox.cx;
    double y = fill_cy - bbox.cy;
    output.add_point({ x, y });

    gosper_walk_iterative(level, x, y, output);
}

void FillFlowsnake::generate(coord_t min_x, coord_t min_y, coord_t max_x, coord_t max_y, const double /* resolution */, InfillPolylineOutput &output)
{
    if (output.clips())
        generate_flowsnake(min_x, min_y, max_x, max_y, static_cast<InfillPolylineClipper&>(output));
    else
        generate_flowsnake(min_x, min_y, max_x, max_y, output);
}

} // namespace Slic3r
