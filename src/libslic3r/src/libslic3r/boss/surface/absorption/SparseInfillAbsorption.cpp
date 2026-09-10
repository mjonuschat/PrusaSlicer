///|/ Copyright (c) preFlight 2026 oozeBot R&D @oozebot
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "libslic3r/boss/surface/absorption/SparseInfillAbsorption.hpp"

#include <algorithm>
#include <cmath>

#include "libslic3r/ClipperUtils.hpp"
#include "libslic3r/Polygon.hpp"
#include "libslic3r/libslic3r.h"
#include "Slic3r/Biz/Algorithms/ExPolygon.hpp"

namespace Slic3r::Boss::SparseInfillAbsorption {

using Slic3r::Biz::Algorithms::ExPolygon::to_polygons;

namespace {

double total_area(const ExPolygons &expolygons)
{
    double area = 0;
    for (const ExPolygon &ep : expolygons)
        area += std::abs(ep.area());
    return area;
}

// The sparse fill's actual line spacing, adjusted for density -- the basis
// for every area/erosion threshold below.
struct SparseThreshold
{
    double min_area     = 0;
    float  erode_radius = 0;
};

SparseThreshold find_sparse_threshold(const std::vector<SurfaceFill> &surface_fills)
{
    for (const SurfaceFill &sf : surface_fills)
        if (sf.surface.surface_type == stInternal && !sf.expolygons.empty() && sf.params.density < 99.f) {
            const float line_spacing = float(scale_(sf.params.spacing)) / (sf.params.density / 100.f);
            return {double(line_spacing) * double(line_spacing) * 4.0, line_spacing * 0.75f};
        }
    return {};
}

// mark_as_infill_above_bridge() assigns different bridge_angles to
// fragments, so group_fills() places them in separate SurfaceFill entries.
// Merge them into the largest one so every later step (hole removal,
// absorption, grow/union/shrink) sees a single unified region.
void consolidate_solid_over_bridge(std::vector<SurfaceFill> &surface_fills)
{
    SurfaceFill *primary   = nullptr;
    double       primary_area = 0;
    for (SurfaceFill &sf : surface_fills) {
        if (sf.expolygons.empty() || sf.surface.surface_type != stSolidOverBridge)
            continue;
        if (const double area = total_area(sf.expolygons); !primary || area > primary_area) {
            primary      = &sf;
            primary_area = area;
        }
    }
    if (!primary)
        return;
    for (SurfaceFill &sf : surface_fills) {
        if (&sf == primary || sf.expolygons.empty() || sf.surface.surface_type != stSolidOverBridge)
            continue;
        append(primary->expolygons, std::move(sf.expolygons));
        sf.expolygons.clear();
    }
    primary->expolygons = union_ex(primary->expolygons);
}

// Removes holes in stInternalSolid that are too small (or, if large but
// thin, fail an erosion test) for sparse fill to cover, unless another
// fill occupies them or they extend past the true fill boundary -- both of
// which mark a real model feature rather than a trimming artifact. A
// feature that fragments stInternalSolid by pattern (e.g.
// narrow-solid-infill-erosion) can split one physical hole's boundary
// across multiple entries, so the keep/remove decision is made once per
// reunified hole shape -- not per entry's own fragment, which alone may
// look too small or too thin even though the real feature is neither --
// and then applied to every entry that shares a piece of it.
void remove_small_internal_solid_holes(std::vector<SurfaceFill> &surface_fills, const SparseThreshold &threshold,
                                        const ExPolygons &total_fill_boundary)
{
    if (threshold.min_area <= 0)
        return;

    Polygons other_fill_polys;
    Polygons all_hole_contours;
    for (const SurfaceFill &sf : surface_fills) {
        if (sf.expolygons.empty())
            continue;
        if (sf.surface.surface_type != stInternalSolid && sf.surface.surface_type != stInternal)
            append(other_fill_polys, to_polygons(sf.expolygons));
        if (sf.surface.surface_type == stInternalSolid)
            for (const ExPolygon &ep : sf.expolygons)
                for (const Polygon &hole : ep.holes) {
                    Polygon contour = hole;
                    contour.reverse();
                    all_hole_contours.push_back(std::move(contour));
                }
    }
    if (all_hole_contours.empty())
        return;

    // NOTE: this merges every stInternalSolid hole that touches, overlaps, or is
    // nested inside another one -- not just genuine fragments of a hole split by
    // a sibling feature. A hole nested inside another entry's hole (e.g. a
    // pillar's through-hole sitting inside the surrounding pocket's outline,
    // when narrow-solid-infill-erosion puts the pillar in a separate entry) is
    // ordinary topology in this composition, not a rare coincidence, and gets
    // silently absorbed into the outer hole's evaluation. A precise fix would
    // need hole-provenance tracking, which the data model doesn't carry --
    // accepted trade-off for now; the failure mode is bounded (one hole kept or
    // dropped that should have gone the other way, never an area-accounting
    // loss or a crash).
    const ExPolygons combined_holes = union_ex(all_hole_contours);

    ExPolygons holes_to_remove;
    for (const ExPolygon &hole_shape : combined_holes) {
        const Polygon &contour = hole_shape.contour;
        bool           remove  = true;
        // Keep large holes unless they're too thin for sparse fill.
        if (std::abs(contour.area()) >= threshold.min_area &&
            (threshold.erode_radius <= 0 || !opening_ex(ExPolygons{ExPolygon(contour)}, threshold.erode_radius).empty()))
            remove = false;
        // Keep the hole if another fill occupies it.
        if (remove && !intersection_ex(ExPolygons{ExPolygon(contour)}, other_fill_polys).empty())
            remove = false;
        // Keep the hole if it extends outside the fill boundary -- a real
        // model feature (a through-hole), not a trimming artifact.
        if (remove) {
            if (ExPolygons outside = diff_ex(ExPolygons{ExPolygon(contour)}, total_fill_boundary);
                !outside.empty() && total_area(outside) > std::abs(contour.area()) * 0.1)
                remove = false;
        }
        if (remove)
            holes_to_remove.push_back(ExPolygon(contour));
    }
    if (holes_to_remove.empty())
        return;

    for (SurfaceFill &fill : surface_fills) {
        if (fill.expolygons.empty() || fill.surface.surface_type != stInternalSolid)
            continue;
        for (ExPolygon &ep : fill.expolygons)
            ep.holes.erase(
                std::remove_if(ep.holes.begin(), ep.holes.end(),
                                [&](const Polygon &hole) {
                                    Polygon contour = hole;
                                    contour.reverse();
                                    return diff_ex(ExPolygons{ExPolygon(contour)}, holes_to_remove).empty();
                                }),
                ep.holes.end());
    }
}

// Removes holes in stSolidOverBridge that vanish under an erosion test --
// arcs and crescents too thin for any fill to produce lines -- while
// keeping thick ones (through-holes shared with other bridge fills).
void remove_thin_solid_over_bridge_holes(std::vector<SurfaceFill> &surface_fills, const SparseThreshold &threshold)
{
    if (threshold.erode_radius <= 0)
        return;
    for (SurfaceFill &fill : surface_fills) {
        if (fill.surface.surface_type != stSolidOverBridge || fill.expolygons.empty())
            continue;
        for (ExPolygon &ep : fill.expolygons) {
            Polygons kept_holes;
            for (const Polygon &hole : ep.holes) {
                Polygon contour = hole;
                contour.reverse();
                if (!opening_ex(ExPolygons{ExPolygon(contour)}, threshold.erode_radius).empty())
                    kept_holes.push_back(hole);
            }
            ep.holes = std::move(kept_holes);
        }
    }
}

// Surface classification splits solid areas into stSolidOverBridge (above
// bridge) and stInternalSolid (other). Where these are adjacent, filling
// them separately leaves thin gaps too narrow for sparse fill. Transfer
// stInternalSolid that physically touches stSolidOverBridge into it, so
// the later grow/union/shrink step can heal the gap.
void transfer_touching_internal_solid_into_bridge(std::vector<SurfaceFill> &surface_fills)
{
    SurfaceFill *bridge_fill = nullptr;
    for (SurfaceFill &sf : surface_fills)
        if (sf.surface.surface_type == stSolidOverBridge && !sf.expolygons.empty()) {
            bridge_fill = &sf;
            break;
        }
    if (!bridge_fill)
        return;

    // Grow slightly to detect touching/near-touching pieces; 0.1mm bridges
    // classification micro-gaps without reaching distant pieces.
    const Polygons bridge_grown = offset(to_polygons(bridge_fill->expolygons), scale_(0.1));

    bool merged = false;
    for (SurfaceFill &sf : surface_fills) {
        if (&sf == bridge_fill || sf.surface.surface_type != stInternalSolid || sf.expolygons.empty())
            continue;
        ExPolygons to_transfer;
        ExPolygons to_keep;
        for (const ExPolygon &ep : sf.expolygons)
            (intersection_ex(ExPolygons{ep}, bridge_grown).empty() ? to_keep : to_transfer).push_back(ep);
        if (!to_transfer.empty()) {
            sf.expolygons = std::move(to_keep);
            append(bridge_fill->expolygons, std::move(to_transfer));
            merged = true;
        }
    }
    if (merged)
        bridge_fill->expolygons = union_ex(bridge_fill->expolygons);
}

// After stSolidOverBridge modifications (hole removal + adjacent-region
// transfer), its expanded coverage may overlap remaining stInternalSolid
// and stInternal fills -- re-trim them against it.
void retrim_against_expanded_solid_over_bridge(std::vector<SurfaceFill> &surface_fills)
{
    Polygons bridge_polys;
    for (const SurfaceFill &sf : surface_fills)
        if (sf.surface.surface_type == stSolidOverBridge && !sf.expolygons.empty())
            append(bridge_polys, to_polygons(sf.expolygons));
    if (bridge_polys.empty())
        return;
    for (SurfaceFill &sf : surface_fills)
        if (!sf.expolygons.empty() && (sf.surface.surface_type == stInternalSolid || sf.surface.surface_type == stInternal))
            sf.expolygons = diff_ex(sf.expolygons, bridge_polys);
}

// Absorbs sparse (stInternal) pockets too small for a meaningful sparse
// fill grid into a solid fill when they sit fully inside it, then merges
// the resulting solid fragments back together across the inter-fragment
// gaps that absorption and prior steps may have left behind.
void absorb_small_sparse_pockets(std::vector<SurfaceFill> &surface_fills, const SparseThreshold &threshold,
                                  const ExPolygons &total_fill_boundary)
{
    for (SurfaceFill &solid_fill : surface_fills) {
        if (solid_fill.expolygons.empty() ||
            (solid_fill.surface.surface_type != stInternalSolid && solid_fill.surface.surface_type != stSolidOverBridge))
            continue;

        // The "filled" solid boundary, contours only (no holes). For
        // stInternalSolid the contours already encompass the sparse
        // pockets. For stSolidOverBridge, mark_as_infill_above_bridge()
        // fragments the solid into disjoint pieces with sparse pockets in
        // the gaps between them -- morphological closing bridges those
        // gaps to reconstruct the encompassing boundary.
        ExPolygons solid_filled;
        if (solid_fill.surface.surface_type == stSolidOverBridge && threshold.erode_radius > 0) {
            Polygons bridge_contours;
            for (const ExPolygon &ep : solid_fill.expolygons)
                bridge_contours.push_back(ep.contour);
            solid_filled = closing_ex(bridge_contours, threshold.erode_radius);
        } else {
            solid_filled.reserve(solid_fill.expolygons.size());
            for (const ExPolygon &ep : solid_fill.expolygons)
                solid_filled.emplace_back(ep.contour);
            solid_filled = union_ex(solid_filled);
        }

        for (SurfaceFill &sparse_fill : surface_fills) {
            if (sparse_fill.surface.surface_type != stInternal || sparse_fill.expolygons.empty() ||
                sparse_fill.params.density >= 99.f)
                continue;

            // A region needs at least a 2x2 grid of sparse lines to be useful.
            const float  line_spacing = float(scale_(sparse_fill.params.spacing)) / (sparse_fill.params.density / 100.f);
            const double min_area     = double(line_spacing) * double(line_spacing) * 4.0;

            ExPolygons to_absorb;
            ExPolygons to_keep;
            for (const ExPolygon &ep : sparse_fill.expolygons) {
                const double area = std::abs(ep.area());
                if (area >= min_area) {
                    to_keep.push_back(ep);
                    continue;
                }
                // A small sparse region counts as an internal pocket -- and
                // gets absorbed -- when the solid fill covers at least 90%
                // of its area.
                if (total_area(intersection_ex(ExPolygons{ep}, solid_filled)) >= area * 0.9)
                    to_absorb.push_back(ep);
                else
                    to_keep.push_back(ep);
            }

            if (!to_absorb.empty()) {
                sparse_fill.expolygons = std::move(to_keep);
                append(solid_fill.expolygons, std::move(to_absorb));
                solid_fill.expolygons = union_ex(solid_fill.expolygons);
            }
        }

        // Grow/union/shrink bridges micro-gaps between fragments that a
        // plain union can't. stSolidOverBridge uses the sparse erosion
        // radius (gaps proportional to sparse spacing); stInternalSolid
        // uses one extrusion width (small classification gaps).
        if (solid_fill.expolygons.size() > 1) {
            const float merge_delta = (solid_fill.surface.surface_type == stSolidOverBridge && threshold.erode_radius > 0)
                                           ? threshold.erode_radius
                                           : float(scale_(solid_fill.params.flow.width()));
            Polygons grown;
            for (const ExPolygon &ep : solid_fill.expolygons)
                append(grown, offset(ep, merge_delta));
            solid_fill.expolygons = intersection_ex(offset_ex(union_(grown), -merge_delta), total_fill_boundary);
        }
    }
}

// After grow/union/shrink, solid fills may have expanded into adjacent
// fills. Re-trim solid fills against each other (priority to earlier
// entries), then sparse fills against every expanded solid fill.
void retrim_after_merge(std::vector<SurfaceFill> &surface_fills)
{
    Polygons processed_solid;
    for (SurfaceFill &sf : surface_fills) {
        if (sf.expolygons.empty() || (sf.surface.surface_type != stInternalSolid && sf.surface.surface_type != stSolidOverBridge))
            continue;
        if (!processed_solid.empty())
            sf.expolygons = diff_ex(sf.expolygons, processed_solid);
        append(processed_solid, to_polygons(sf.expolygons));
    }
    if (!processed_solid.empty())
        for (SurfaceFill &sf : surface_fills)
            if (sf.surface.surface_type == stInternal && !sf.expolygons.empty())
                sf.expolygons = diff_ex(sf.expolygons, processed_solid);
}

// The grow/union/shrink merge can leave tiny stSolidOverBridge fragments
// near tight features (screw holes, pegs) that overlap perimeters when
// filled. stSolidOverBridge is 100% density, so even a small area produces
// valid fill lines -- the sparse threshold would be too large and delete
// legitimate regions, so this uses the solid fill's own spacing instead.
void remove_tiny_solid_over_bridge_fragments(std::vector<SurfaceFill> &surface_fills)
{
    double solid_min_area = 0;
    for (const SurfaceFill &sf : surface_fills)
        if (sf.surface.surface_type == stSolidOverBridge && !sf.expolygons.empty()) {
            const double solid_spacing = scale_(sf.params.spacing);
            solid_min_area             = solid_spacing * solid_spacing * 4.0; // 2x2 line grid.
            break;
        }
    if (solid_min_area <= 0)
        return;
    for (SurfaceFill &sf : surface_fills)
        if (sf.surface.surface_type == stSolidOverBridge && !sf.expolygons.empty())
            sf.expolygons.erase(std::remove_if(sf.expolygons.begin(), sf.expolygons.end(),
                                                [solid_min_area](const ExPolygon &ep) { return std::abs(ep.area()) < solid_min_area; }),
                                 sf.expolygons.end());
}

} // namespace

void absorb(std::vector<SurfaceFill> &surface_fills, const ExPolygons &total_fill_boundary)
{
    const SparseThreshold threshold = find_sparse_threshold(surface_fills);

    consolidate_solid_over_bridge(surface_fills);
    remove_small_internal_solid_holes(surface_fills, threshold, total_fill_boundary);
    remove_thin_solid_over_bridge_holes(surface_fills, threshold);
    transfer_touching_internal_solid_into_bridge(surface_fills);
    retrim_against_expanded_solid_over_bridge(surface_fills);
    absorb_small_sparse_pockets(surface_fills, threshold, total_fill_boundary);
    retrim_after_merge(surface_fills);
    remove_tiny_solid_over_bridge_fragments(surface_fills);
}

} // namespace Slic3r::Boss::SparseInfillAbsorption
