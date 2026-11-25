///|/ Copyright (c) 2024 - 2026 Morton Jonuschat @mjonuschat
///|/ Copyright (c) preFlight 2026 oozeBot R&D @oozebot
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
// Ported from BOSS 901d98a8c9 (src/libslic3r/GCode.cpp, preFlight's Nip/Tuck
// seam hiding). Adapted to read config through a Domain::ConfigView instead
// of FullPrintConfig, and to operate on GCode::ExtrusionOrder::Perimeter
// vectors passed in explicitly instead of GCodeGenerator member state.
#include "boss/features/nip-tuck-seam/NipTuckSeamFeature.hpp"
#include "boss/foundation/PerimeterGeometryContext.hpp"
#include "libslic3r/boss/seam/NipTuckSeamNotch.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "Slic3r/Domain/Config.hpp"
#include "libslic3r/ExtrusionEntity.hpp"
#include "libslic3r/GCode/SmoothPath.hpp"
#include "libslic3r/Geometry/ArcWelder.hpp"

namespace Slic3r::Boss {

bool NipTuckSeamFeature::suppress_staggering(const Domain::ConfigView &config, std::optional<int> perimeter_index)
{
    if (config.get<NipTuckSeamType>("seam_type") == NipTuckSeamType::Regular)
        return false;
    return perimeter_index.has_value() && *perimeter_index == 1;
}

namespace {

using GCode::ExtrusionOrder::Perimeter;

// Subdivide segments in a SmoothPath region to ensure no segment exceeds max_len (scaled).
// Operates on the path in-place. Returns the number of points inserted.
size_t subdivide_smooth_path_region(Geometry::ArcWelder::Path &path, size_t start_idx, size_t end_idx, double max_len)
{
    size_t inserted = 0;
    for (size_t i = start_idx; i < end_idx && i < path.size(); ++i) {
        if (i == 0)
            continue;
        const Vec2d seg = (path[i].point - path[i - 1].point).cast<double>();
        const double seg_len = seg.norm();
        if (seg_len > max_len * 1.5) {
            const int n_splits = static_cast<int>(std::ceil(seg_len / max_len));
            const Vec2d step = seg / n_splits;
            const float orig_e_fraction = path[i].e_fraction;
            const float orig_h_fraction = path[i].height_fraction;
            std::vector<Geometry::ArcWelder::Segment> new_segs;
            for (int j = 1; j < n_splits; ++j) {
                Geometry::ArcWelder::Segment s;
                s.point = path[i - 1].point + (step * j).cast<coord_t>();
                s.radius = 0; // linearize
                s.e_fraction = orig_e_fraction;
                s.height_fraction = orig_h_fraction;
                new_segs.push_back(s);
            }
            path.insert(path.begin() + i, new_segs.begin(), new_segs.end());
            const size_t count = new_segs.size();
            inserted += count;
            end_idx += count;
            i += count; // skip past inserted points
        }
    }
    return inserted;
}

// Compute the inward direction at the seam point of an external perimeter loop.
// The seam is at path start (first point) and path end (last point) of the open SmoothPath.
// Returns a unit vector pointing toward the interior of the part.
Vec2d compute_seam_inward_direction(const GCode::SmoothPath &smooth_path, bool reversed)
{
    Vec2d dir_start = Vec2d::Zero();
    for (const auto &elem : smooth_path) {
        if (elem.path.size() >= 2) {
            dir_start = (elem.path[1].point - elem.path[0].point).cast<double>();
            break;
        }
    }

    Vec2d dir_end = Vec2d::Zero();
    for (auto it = smooth_path.rbegin(); it != smooth_path.rend(); ++it) {
        if (it->path.size() >= 2) {
            const auto &p = it->path;
            dir_end = (p[p.size() - 1].point - p[p.size() - 2].point).cast<double>();
            break;
        }
    }

    if (dir_start.squaredNorm() < 1e-10 || dir_end.squaredNorm() < 1e-10)
        return Vec2d::Zero();

    dir_start.normalize();
    dir_end.normalize();

    Vec2d bisect = (dir_start - dir_end);
    if (bisect.squaredNorm() < 1e-10) {
        bisect = Vec2d(-dir_start.y(), dir_start.x());
    }
    bisect.normalize();

    Vec2d left_of_start = Vec2d(-dir_start.y(), dir_start.x());
    if (reversed)
        left_of_start = -left_of_start;

    if (bisect.dot(left_of_start) < 0)
        bisect = -bisect;

    return bisect;
}

// Apply V-notch offset to the start and end of an external perimeter's SmoothPath.
// half_width_scaled: half the notch width in scaled coordinates
// depth_scaled: maximum inward offset in scaled coordinates
// inward: unit vector pointing toward part interior
void apply_notch_to_external(GCode::SmoothPath &smooth_path, double half_width_scaled, double depth_scaled,
                              const Vec2d &inward, NipTuckSeamType notch_type)
{
    if (smooth_path.empty() || inward.squaredNorm() < 1e-10)
        return;

    const double max_seg_len = half_width_scaled * 0.15; // ~15% of half width for smooth taper

    // --- Taper from path START (seam start) ---
    // Tuck mode skips the start taper (only the end cuts inward).
    if (notch_type != NipTuckSeamType::Tuck) {
        auto &first_path = smooth_path.front().path;

        size_t taper_end_idx = 1;
        double accum_dist = 0;
        for (size_t i = 1; i < first_path.size(); ++i) {
            double seg_len = (first_path[i].point - first_path[i - 1].point).cast<double>().norm();
            if (accum_dist + seg_len >= half_width_scaled) {
                double remaining = half_width_scaled - accum_dist;
                double frac = (seg_len > 1e-6) ? (remaining / seg_len) : 1.0;
                if (frac > 0.001 && frac < 0.999) {
                    Geometry::ArcWelder::Segment s;
                    s.point = first_path[i - 1].point +
                              ((first_path[i].point - first_path[i - 1].point).cast<double>() * frac).cast<coord_t>();
                    s.radius = 0;
                    s.e_fraction = first_path[i].e_fraction;
                    s.height_fraction = first_path[i].height_fraction;
                    first_path.insert(first_path.begin() + i, s);
                }
                taper_end_idx = i;
                break;
            }
            accum_dist += seg_len;
            taper_end_idx = i;
        }

        for (size_t i = 1; i <= taper_end_idx && i < first_path.size(); ++i)
            first_path[i].radius = 0;

        subdivide_smooth_path_region(first_path, 1, taper_end_idx + 1, max_seg_len);

        double dist_from_start = 0;
        for (size_t i = 0; i < first_path.size(); ++i) {
            if (i > 0)
                dist_from_start += (first_path[i].point - first_path[i - 1].point).cast<double>().norm();
            if (dist_from_start > half_width_scaled)
                break;
            double t = (half_width_scaled > 0) ? (dist_from_start / half_width_scaled) : 1.0;
            t = std::min(t, 1.0);
            double offset = depth_scaled * (1.0 - t);
            first_path[i].point += (inward * offset).cast<coord_t>();
        }
    }

    // --- Taper from path END (seam end) ---
    // Nip mode skips the end taper (only the start cuts inward).
    if (notch_type != NipTuckSeamType::Nip) {
        auto &last_path = smooth_path.back().path;
        if (last_path.size() < 2)
            return;

        size_t taper_start_idx = last_path.size() - 2;
        double accum_dist = 0;
        for (int i = static_cast<int>(last_path.size()) - 1; i > 0; --i) {
            double seg_len = (last_path[i].point - last_path[i - 1].point).cast<double>().norm();
            if (accum_dist + seg_len >= half_width_scaled) {
                double remaining = half_width_scaled - accum_dist;
                double frac_from_end = (seg_len > 1e-6) ? (remaining / seg_len) : 1.0;
                double frac_from_start = 1.0 - frac_from_end;
                if (frac_from_start > 0.001 && frac_from_start < 0.999) {
                    Geometry::ArcWelder::Segment s;
                    s.point = last_path[i - 1].point +
                              ((last_path[i].point - last_path[i - 1].point).cast<double>() * frac_from_start)
                                  .cast<coord_t>();
                    s.radius = 0;
                    s.e_fraction = last_path[i].e_fraction;
                    s.height_fraction = last_path[i].height_fraction;
                    last_path.insert(last_path.begin() + i, s);
                }
                taper_start_idx = static_cast<size_t>(i);
                break;
            }
            accum_dist += seg_len;
            taper_start_idx = static_cast<size_t>(i - 1);
        }

        for (size_t i = taper_start_idx + 1; i < last_path.size(); ++i)
            last_path[i].radius = 0;

        subdivide_smooth_path_region(last_path, taper_start_idx + 1, last_path.size(), max_seg_len);

        double dist_from_end = 0;
        for (int i = static_cast<int>(last_path.size()) - 1; i >= 0; --i) {
            if (i < static_cast<int>(last_path.size()) - 1)
                dist_from_end += (last_path[i + 1].point - last_path[i].point).cast<double>().norm();
            if (dist_from_end > half_width_scaled)
                break;
            double t = (half_width_scaled > 0) ? (dist_from_end / half_width_scaled) : 1.0;
            t = std::min(t, 1.0);
            double offset = depth_scaled * (1.0 - t);
            last_path[i].point += (inward * offset).cast<coord_t>();
        }
    }
}

// Extend a path past its last point in the direction of the last segment.
void extend_path_end(GCode::SmoothPath &smooth_path, double extension_scaled)
{
    if (smooth_path.empty() || smooth_path.back().path.size() < 2)
        return;
    auto &last_path = smooth_path.back().path;
    const Vec2d dir = (last_path.back().point - last_path[last_path.size() - 2].point).cast<double>().normalized();
    Geometry::ArcWelder::Segment s;
    s.point = last_path.back().point + (dir * extension_scaled).cast<coord_t>();
    s.radius = 0;
    s.e_fraction = last_path.back().e_fraction;
    s.height_fraction = last_path.back().height_fraction;
    last_path.push_back(s);
}

// Extend a path before its first point in the reverse direction of the first segment.
void extend_path_start(GCode::SmoothPath &smooth_path, double extension_scaled)
{
    if (smooth_path.empty() || smooth_path.front().path.size() < 2)
        return;
    auto &first_path = smooth_path.front().path;
    const Vec2d dir = (first_path[0].point - first_path[1].point).cast<double>().normalized();
    Geometry::ArcWelder::Segment s;
    s.point = first_path[0].point + (dir * extension_scaled).cast<coord_t>();
    s.radius = 0;
    s.e_fraction = first_path[0].e_fraction;
    s.height_fraction = first_path[0].height_fraction;
    first_path.insert(first_path.begin(), s);
}

// Adjust the end of a path: positive values clip, negative values extend.
void adjust_path_end(GCode::SmoothPath &smooth_path, double adjustment_scaled)
{
    if (adjustment_scaled > 0) {
        const double min_threshold = scaled<double>(GCode::ExtrusionOrder::min_gcode_segment_length);
        GCode::clip_end(smooth_path, adjustment_scaled, min_threshold);
    } else if (adjustment_scaled < 0) {
        extend_path_end(smooth_path, -adjustment_scaled);
    }
}

// Adjust the start of a path: positive values clip, negative values extend.
void adjust_path_start(GCode::SmoothPath &smooth_path, double adjustment_scaled)
{
    if (adjustment_scaled > 0) {
        const double min_threshold = scaled<double>(GCode::ExtrusionOrder::min_gcode_segment_length);
        GCode::reverse(smooth_path);
        GCode::clip_end(smooth_path, adjustment_scaled, min_threshold);
        GCode::reverse(smooth_path);
    } else if (adjustment_scaled < 0) {
        extend_path_start(smooth_path, -adjustment_scaled);
    }
}

// Adjust the inner perimeter at the notch location. Start and end adjustments
// can differ for asymmetric modes (Nip-only, Tuck-only).
void apply_notch_to_inner(GCode::SmoothPath &smooth_path, double start_adjust_scaled, double end_adjust_scaled)
{
    if (smooth_path.empty())
        return;
    adjust_path_end(smooth_path, end_adjust_scaled);
    adjust_path_start(smooth_path, start_adjust_scaled);
}

// Compute the notch trim distance for the inner perimeter gap (in scaled units).
// This is the distance to clip from each side of the split to create the gap.
// Mirrors the inner-trim math in apply_seam_notch_pair so a split inner produces
// the same gap geometry as a notched one.
//
// The original ported spacing math resolved external_perimeter_overlap to
// absolute mm and subtracted it from ext_width. That option belongs to a
// separate, independently-composable BOSS feature (perimeter-overlap) that
// this feature must not depend on, so spacing here is simply ext_width
// (equivalent to that feature contributing zero overlap when not composed).
double compute_inner_notch_trim(double ext_width, double inner_width, double seam_notch_width)
{
    const double notch_width_mm = seam_notch_width * ext_width;
    const double notch_depth = ext_width * 0.9;
    const double spacing = ext_width;

    const double x_int = (notch_depth > spacing && notch_width_mm > 0)
                              ? notch_width_mm * (1.0 - spacing / notch_depth)
                              : 0.0;

    const double base_trim = x_int + inner_width / 2.0;
    const double notch_trim_mm = std::max(0.0, base_trim * (0.65 + 0.35 * seam_notch_width));
    return scale_(notch_trim_mm);
}

// Split a smooth path at the point closest to 'target', returning the second half.
// The first half remains in 'smooth_path'. The second half is returned.
// trim_first_end: distance to clip from end of first half (at split gap).
// trim_second_start: distance to clip from start of second half (at split gap).
// inner_fwd (out): path forward direction at the split point (for V-leg side detection).
GCode::SmoothPath split_smooth_path_near_point(GCode::SmoothPath &smooth_path, const Point &target,
                                                double trim_first_end, double trim_second_start, Vec2d &inner_fwd)
{
    GCode::SmoothPath second_half;
    inner_fwd = Vec2d::Zero();
    if (smooth_path.empty())
        return second_half;

    size_t best_elem = 0;
    size_t best_seg = 0;
    double best_dist_sq = std::numeric_limits<double>::max();
    Point best_point = Point(0, 0);

    for (size_t ei = 0; ei < smooth_path.size(); ++ei) {
        const auto &path = smooth_path[ei].path;
        for (size_t si = 1; si < path.size(); ++si) {
            Vec2d a = path[si - 1].point.cast<double>();
            Vec2d b = path[si].point.cast<double>();
            Vec2d ab = b - a;
            double len_sq = ab.squaredNorm();

            double t = 0;
            if (len_sq > 1e-10)
                t = std::clamp((target.cast<double>() - a).dot(ab) / len_sq, 0.0, 1.0);

            Vec2d closest = a + ab * t;
            double dist_sq = (target.cast<double>() - closest).squaredNorm();
            if (dist_sq < best_dist_sq) {
                best_dist_sq = dist_sq;
                best_elem = ei;
                best_seg = si;
                best_point = (a + ab * t).cast<coord_t>();
            }
        }
    }

    if (best_dist_sq == std::numeric_limits<double>::max())
        return second_half;

    {
        const auto &path = smooth_path[best_elem].path;
        Vec2d seg_dir = (path[best_seg].point - path[best_seg - 1].point).cast<double>();
        if (seg_dir.squaredNorm() > 1e-10)
            inner_fwd = seg_dir.normalized();
    }

    Geometry::ArcWelder::Segment split_seg;
    split_seg.point = best_point;
    split_seg.radius = 0; // linearize at split
    split_seg.e_fraction = smooth_path[best_elem].path[best_seg].e_fraction;
    split_seg.height_fraction = smooth_path[best_elem].path[best_seg].height_fraction;

    {
        GCode::SmoothPathElement first_elem;
        first_elem.path_attributes = smooth_path[best_elem].path_attributes;
        first_elem.path.push_back(split_seg);
        for (size_t si = best_seg; si < smooth_path[best_elem].path.size(); ++si)
            first_elem.path.push_back(smooth_path[best_elem].path[si]);
        second_half.push_back(std::move(first_elem));
    }
    for (size_t ei = best_elem + 1; ei < smooth_path.size(); ++ei)
        second_half.push_back(smooth_path[ei]);

    smooth_path.resize(best_elem + 1);
    auto &last_path = smooth_path[best_elem].path;
    last_path.resize(best_seg);
    last_path.push_back(split_seg);

    const double min_threshold = scaled<double>(GCode::ExtrusionOrder::min_gcode_segment_length);
    if (trim_first_end > 0)
        GCode::clip_end(smooth_path, trim_first_end, min_threshold);
    if (trim_second_start > 0) {
        GCode::reverse(second_half);
        GCode::clip_end(second_half, trim_second_start, min_threshold);
        GCode::reverse(second_half);
    }

    return second_half;
}

} // namespace

// Apply seam notch to a single (external, inner) perimeter pair.
// Modifies the external perimeter and optionally the first inner perimeter in-place.
void apply_seam_notch_pair(
    GCode::ExtrusionOrder::Perimeter &ext_perim,
    GCode::ExtrusionOrder::Perimeter *inner_perim,
    const Domain::ConfigView &config,
    int layer_index)
{
    // Check corner sharpness at the seam -- sharp corners naturally hide seams.
    // Extract the path directions arriving at and leaving the seam point, same vectors
    // that compute_seam_inward_direction() uses. The dot product of these normalized
    // directions gives cos(deviation_from_straight). Skip if the corner is sharper
    // than the configured threshold.
    const double corner_threshold_deg = config.get<double>("seam_notch_angle");
    if (corner_threshold_deg > 0) {
        Vec2d dir_start = Vec2d::Zero();
        for (const auto &elem : ext_perim.smooth_path)
            if (elem.path.size() >= 2) {
                dir_start = (elem.path[1].point - elem.path[0].point).cast<double>();
                break;
            }

        Vec2d dir_end = Vec2d::Zero();
        for (auto it = ext_perim.smooth_path.rbegin(); it != ext_perim.smooth_path.rend(); ++it)
            if (it->path.size() >= 2) {
                const auto &p = it->path;
                dir_end = (p[p.size() - 1].point - p[p.size() - 2].point).cast<double>();
                break;
            }

        if (dir_start.squaredNorm() > 1e-10 && dir_end.squaredNorm() > 1e-10) {
            dir_start.normalize();
            dir_end.normalize();
            const double cos_deviation = dir_start.dot(dir_end);
            const double cos_threshold = std::cos(corner_threshold_deg * M_PI / 180.0);
            if (cos_deviation < cos_threshold)
                return; // Corner is sharp enough to hide the seam naturally
        }
    }

    // Get external perimeter extrusion width (needed for notch width and depth calculations)
    const auto *ext_loop = dynamic_cast<const ExtrusionLoop *>(ext_perim.extrusion_entity);
    double ext_width = 0;
    if (ext_loop != nullptr && !ext_loop->paths.empty())
        ext_width = ext_loop->paths.front().width();
    if (ext_width <= 0)
        ext_width = config.get<std::vector<double>>("nozzle_diameter").at(0);

    const double seam_notch_width = config.get<double>("seam_notch_width");

    // Notch width is a multiple of external perimeter extrusion width
    const double notch_width_mm = seam_notch_width * ext_width;
    const double half_width_scaled = scale_(notch_width_mm);

    // Check minimum loop length -- don't notch tiny features
    const double min_loop_length = scale_(notch_width_mm * 3.0);
    double ext_loop_len = 0;
    for (const auto &elem : ext_perim.smooth_path)
        for (size_t i = 1; i < elem.path.size(); ++i)
            ext_loop_len += (elem.path[i].point - elem.path[i - 1].point).cast<double>().norm();
    if (ext_loop_len < min_loop_length)
        return;

    // The center-to-center spacing between ext and first inner is approximately ext_width
    // (since perimeters are offset by their spacing which is close to width).
    // Use 90% of ext_width as depth to avoid pushing all the way to the inner perimeter.
    const double depth_scaled = scale_(ext_width * 0.9);

    // Compute inward direction from the external perimeter's seam geometry
    Vec2d inward = compute_seam_inward_direction(ext_perim.smooth_path, ext_perim.reversed);
    if (inward.squaredNorm() < 0.5) // degenerate -- can't determine direction
        return;

    // Resolve notch type for this layer (Alternating flips per layer)
    NipTuckSeamType notch_type = config.get<NipTuckSeamType>("seam_type");
    if (notch_type == NipTuckSeamType::Alternating)
        notch_type = (layer_index % 2 == 0) ? NipTuckSeamType::Nip : NipTuckSeamType::Tuck;

    // Apply V-notch to external perimeter
    apply_notch_to_external(ext_perim.smooth_path, half_width_scaled, depth_scaled, inward, notch_type);

    // For asymmetric modes, adjust the non-notched endpoint of the external perimeter
    // to prevent bead overlap or close gaps depending on notch width.
    if (notch_type == NipTuckSeamType::Nip || notch_type == NipTuckSeamType::Tuck) {
        const double ext_adjust = scale_(std::max(0.0, ext_width * (1.0 - 0.5 * seam_notch_width)));
        if (notch_type == NipTuckSeamType::Nip)
            adjust_path_end(ext_perim.smooth_path, ext_adjust);
        else
            adjust_path_start(ext_perim.smooth_path, ext_adjust);
    }

    // Trim the first inner perimeter where the V-leg crosses it.
    if (inner_perim != nullptr) {
        const auto *inner_loop = dynamic_cast<const ExtrusionLoop *>(inner_perim->extrusion_entity);
        double inner_width = 0;
        if (inner_loop != nullptr && !inner_loop->paths.empty())
            inner_width = inner_loop->paths.front().width();
        if (inner_width <= 0)
            inner_width = ext_width;

        const double notch_trim_scaled = compute_inner_notch_trim(ext_width, inner_width, seam_notch_width);
        const double inner_non_notch_clip = scale_(inner_width / 2.0);

        // For asymmetric modes, the notched side gets full trim, the non-notched side
        // gets a smaller clip of half-bead width to prevent overlap at the seam point.
        double start_adjust = notch_trim_scaled;
        double end_adjust = notch_trim_scaled;
        if (notch_type == NipTuckSeamType::Nip)
            end_adjust = inner_non_notch_clip;
        else if (notch_type == NipTuckSeamType::Tuck)
            start_adjust = inner_non_notch_clip;

        apply_notch_to_inner(inner_perim->smooth_path, start_adjust, end_adjust);
    }
}

// Main entry point: apply seam notch to all external perimeters in an island.
// Finds all external perimeters and matches each with its closest inner perimeter (index 1)
// by seam start point proximity, then applies the notch to each pair.
void apply_seam_notch(
    std::vector<GCode::ExtrusionOrder::Perimeter> &perimeters,
    const Domain::ConfigView &config,
    int layer_index)
{
    // Helper to get seam start point from a perimeter
    auto get_seam_point = [](const Perimeter &p) -> Point {
        if (!p.smooth_path.empty() && !p.smooth_path.front().path.empty())
            return p.smooth_path.front().path.front().point;
        return Point(0, 0);
    };

    // Collect all external perimeters and all inner perimeters (index 1)
    std::vector<Perimeter *> ext_perims;
    std::vector<Perimeter *> inner_perims;

    for (auto &p : perimeters) {
        if (p.extrusion_entity == nullptr || p.smooth_path.empty())
            continue;

        const auto *loop = dynamic_cast<const ExtrusionLoop *>(p.extrusion_entity);
        if (loop == nullptr)
            continue;

        if (loop->role().is_external_perimeter()) {
            ext_perims.push_back(&p);
        } else if (loop->role().is_perimeter() && !loop->paths.empty()) {
            auto pi = loop->paths.front().attributes().perimeter_index;
            if (pi.has_value() && *pi == 1)
                inner_perims.push_back(&p);
        }
    }

    // Match each external perimeter with its closest inner perimeter by seam proximity.
    struct ExtInnerPair {
        Perimeter *ext;
        Perimeter *inner;
    };
    std::vector<ExtInnerPair> pairs;

    for (Perimeter *ext : ext_perims) {
        Perimeter *best_inner = nullptr;
        if (!inner_perims.empty()) {
            Point ext_seam = get_seam_point(*ext);
            double best_dist = std::numeric_limits<double>::max();
            for (Perimeter *inner : inner_perims) {
                double dist = (get_seam_point(*inner) - ext_seam).cast<double>().norm();
                if (dist < best_dist) {
                    best_dist = dist;
                    best_inner = inner;
                }
            }
        }
        // Only notch when an inner perimeter exists to absorb the disturbance
        if (best_inner != nullptr)
            pairs.push_back({ext, best_inner});
    }

    // Helper: get external perimeter extrusion width
    auto get_ext_width = [&config](const Perimeter &p) -> double {
        const auto *loop = dynamic_cast<const ExtrusionLoop *>(p.extrusion_entity);
        double w = 0;
        if (loop != nullptr && !loop->paths.empty())
            w = loop->paths.front().width();
        return w > 0 ? w : config.get<std::vector<double>>("nozzle_diameter").at(0);
    };

    // Helper: get inner perimeter extrusion width
    auto get_inner_width = [](const Perimeter &p, double fallback) -> double {
        const auto *loop = dynamic_cast<const ExtrusionLoop *>(p.extrusion_entity);
        double w = 0;
        if (loop != nullptr && !loop->paths.empty())
            w = loop->paths.front().width();
        return w > 0 ? w : fallback;
    };

    // Collect new perimeters for split inner halves (added after the loop to avoid
    // invalidating pointers into the perimeters vector).
    std::vector<Perimeter> new_perimeters;

    // Detect shared inner perimeters (thin wall: 2 externals, 1 inner). Without this,
    // both externals notch the same inner and produce a double notch. When shared, the
    // close external notches the inner at its seam break as normal, then the inner is
    // split at the point nearest the far external's seam to create a second notch gap.
    std::vector<bool> processed(pairs.size(), false);
    for (size_t i = 0; i < pairs.size(); ++i) {
        if (processed[i])
            continue;

        // Find all pairs sharing the same inner perimeter
        std::vector<size_t> sharing;
        for (size_t j = i; j < pairs.size(); ++j)
            if (!processed[j] && pairs[j].inner == pairs[i].inner)
                sharing.push_back(j);

        for (size_t idx : sharing)
            processed[idx] = true;

        if (sharing.size() == 1) {
            // Normal case: this external owns the inner exclusively
            apply_seam_notch_pair(*pairs[i].ext, pairs[i].inner, config, layer_index);
        } else if (sharing.size() == 2) {
            Perimeter *inner = pairs[sharing[0]].inner;
            Point inner_pt = get_seam_point(*inner);

            Point ext0_seam = get_seam_point(*pairs[sharing[0]].ext);
            Point ext1_seam = get_seam_point(*pairs[sharing[1]].ext);
            double dist0 = (ext0_seam - inner_pt).cast<double>().norm();
            double dist1 = (ext1_seam - inner_pt).cast<double>().norm();

            // Identify which external is close to the inner's seam and which is far
            size_t close_idx = (dist0 <= dist1) ? sharing[0] : sharing[1];
            size_t far_idx = (dist0 <= dist1) ? sharing[1] : sharing[0];

            // Close external: normal notch (trims inner at its seam break)
            apply_seam_notch_pair(*pairs[close_idx].ext, inner, config, layer_index);

            // Skip split if the inner is too short after the close external's trim
            const double min_split_len = scale_(get_ext_width(*pairs[far_idx].ext) * 3.0);
            if (!GCode::longer_than(inner->smooth_path, min_split_len)) {
                apply_seam_notch_pair(*pairs[far_idx].ext, nullptr, config, layer_index);
                continue;
            }

            // Far external: split the inner at the point nearest the far external's V-notch.
            // Project from the V-notch's deepest point (not the external's seam on the far
            // side of the wall) for accurate centering on curved/angled walls.
            Point far_seam = get_seam_point(*pairs[far_idx].ext);
            double ew = get_ext_width(*pairs[far_idx].ext);
            double iw = get_inner_width(*inner, ew);

            Vec2d inward = compute_seam_inward_direction(pairs[far_idx].ext->smooth_path, pairs[far_idx].ext->reversed);
            Point split_target = far_seam;
            if (inward.squaredNorm() > 0.5)
                split_target = far_seam + (inward * scale_(ew * 0.9)).cast<coord_t>();

            // Compute asymmetric trim matching the normal notch behavior:
            // V-leg side gets notch_trim, non-V-leg side gets non_notch_clip.
            const double seam_notch_width = config.get<double>("seam_notch_width");
            double notch_trim = compute_inner_notch_trim(ew, iw, seam_notch_width);
            double non_notch_clip = scale_(iw / 2.0);
            NipTuckSeamType snt = config.get<NipTuckSeamType>("seam_type");
            if (snt == NipTuckSeamType::Alternating)
                snt = (layer_index % 2 == 0) ? NipTuckSeamType::Nip : NipTuckSeamType::Tuck;

            // Default: symmetric (both sides get notch_trim)
            double trim_first_end = notch_trim;
            double trim_second_start = notch_trim;

            // For Nip/Tuck, determine which side of the split faces the V-leg
            // by comparing the external's seam direction with the inner's path direction.
            if (snt == NipTuckSeamType::Nip || snt == NipTuckSeamType::Tuck) {
                // External forward direction at seam
                Vec2d ext_fwd = Vec2d::Zero();
                for (const auto &elem : pairs[far_idx].ext->smooth_path)
                    if (elem.path.size() >= 2) {
                        ext_fwd = (elem.path[1].point - elem.path[0].point).cast<double>().normalized();
                        break;
                    }

                // Inner path direction at the closest point to split_target
                Vec2d inner_dir = Vec2d::Zero();
                {
                    double best_d2 = std::numeric_limits<double>::max();
                    for (const auto &elem : inner->smooth_path)
                        for (size_t si = 1; si < elem.path.size(); ++si) {
                            Vec2d a = elem.path[si - 1].point.cast<double>();
                            Vec2d b = elem.path[si].point.cast<double>();
                            Vec2d ab = b - a;
                            double l2 = ab.squaredNorm();
                            double t = (l2 > 1e-10)
                                           ? std::clamp((split_target.cast<double>() - a).dot(ab) / l2, 0.0, 1.0)
                                           : 0.0;
                            double d2 = (split_target.cast<double>() - (a + ab * t)).squaredNorm();
                            if (d2 < best_d2) {
                                best_d2 = d2;
                                inner_dir = ab;
                            }
                        }
                    if (inner_dir.squaredNorm() > 1e-10)
                        inner_dir.normalize();
                }

                if (ext_fwd.squaredNorm() > 0.5 && inner_dir.squaredNorm() > 0.5) {
                    // Nip: V-leg in ext_fwd direction. Tuck: V-leg in -ext_fwd direction.
                    Vec2d vleg_dir = (snt == NipTuckSeamType::Nip) ? ext_fwd : -ext_fwd;
                    bool vleg_is_fwd = vleg_dir.dot(inner_dir) > 0;
                    // Forward on inner = second half's start. Backward = first half's end.
                    trim_first_end = vleg_is_fwd ? non_notch_clip : notch_trim;
                    trim_second_start = vleg_is_fwd ? notch_trim : non_notch_clip;
                }
            }

            Vec2d inner_fwd;
            GCode::SmoothPath second_half = split_smooth_path_near_point(inner->smooth_path, split_target,
                                                                          trim_first_end, trim_second_start, inner_fwd);

            // Clamp wipe_offset to prevent OOB after truncation
            if (inner->wipe_offset > inner->smooth_path.size())
                inner->wipe_offset = 0;

            // Apply external V-notch only for the far external (inner gap handled by split)
            apply_seam_notch_pair(*pairs[far_idx].ext, nullptr, config, layer_index);

            // Add the second half of the split inner as a new perimeter
            if (!second_half.empty()) {
                Perimeter new_perim;
                new_perim.smooth_path = std::move(second_half);
                new_perim.reversed = inner->reversed;
                new_perim.extrusion_entity = inner->extrusion_entity;
                new_perim.wipe_offset = 0;
                new_perimeters.push_back(std::move(new_perim));
            }
        } else {
            // 3+ externals sharing one inner (unlikely) -- only closest gets inner notch
            Perimeter *inner = pairs[sharing[0]].inner;
            Point inner_pt = get_seam_point(*inner);
            size_t closest = sharing[0];
            double best_d = std::numeric_limits<double>::max();
            for (size_t idx : sharing) {
                double d = (get_seam_point(*pairs[idx].ext) - inner_pt).cast<double>().norm();
                if (d < best_d) {
                    best_d = d;
                    closest = idx;
                }
            }
            for (size_t idx : sharing) {
                if (idx == closest)
                    apply_seam_notch_pair(*pairs[idx].ext, inner, config, layer_index);
                else
                    apply_seam_notch_pair(*pairs[idx].ext, nullptr, config, layer_index);
            }
        }
    }

    // Append split inner perimeter halves
    for (auto &p : new_perimeters)
        perimeters.push_back(std::move(p));
}

void NipTuckSeamFeature::modify_perimeters(const PerimeterGeometryContext &ctx)
{
    if (ctx.config == nullptr)
        return;
    if (ctx.config->get<NipTuckSeamType>("seam_type") == NipTuckSeamType::Regular)
        return;
    // "perimeters" has overrides_in {Tool, Object, Volume}, so a ConfigView
    // always resolves it to a per-material-slot vector, never a bare int --
    // index by this island's own extruder slot, the same way PrintRegion.cpp
    // and Layer.cpp read it.
    if (ctx.config->get<std::vector<int>>("perimeters").at(ctx.extruder_id) <= 1
        || ctx.config->get<bool>("spiral_vase"))
        return;
    if (ctx.perimeters.empty())
        return;

    apply_seam_notch(ctx.perimeters, *ctx.config, ctx.layer_index);
}

} // namespace Slic3r::Boss
