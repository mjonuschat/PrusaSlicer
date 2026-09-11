# BOSS Feature Origins

Source tracking for every feature BOSS ported from another slicer. The point is
upstream sync: when a source slicer ships a new release, compare against the
"ported from" commit/version here to find fixes and improvements worth pulling.

This document lives on the orphan `upstream-tracking` branch, parallel to
`release-notes`/`CHANGELOG.md`. Do not merge it into `boss` or `release/*`.

BOSS features are identified by commit subject (stable across rebases), not hash.
BOSS-original features (no external source) are listed at the bottom and need no
sync monitoring.

## Source sync points

| Slicer | Local checkout | Last synced |
|--------|----------------|-------------|
| SuperSlicer | `../SuperSlicer` | tag `2.5.59.13` (`e2eef1da2d`, 2024-07-01) |
| OrcaSlicer | `../OrcaSlicer` | tag `v2.4.0` (`6d9eb1792f`, 2026-06-19) |
| preFlight | `../preFlight` | v1.0.1-beta2 (`99f70c2`, 2026-05-28) |

preFlight publishes one squashed commit per release, so its finest granularity is
the release commit (= version tag). Per-PR history is not public; the CHANGELOG on
each release gives feature-level detail.

## SuperSlicer

| BOSS feature | SuperSlicer key | Origin commit | Author / date |
|--------------|-----------------|---------------|---------------|
| Z-rotate on import | `init_z_rotate` | `a0f8d5023a` | Christoph Schöning, 2021-12-22 |
| Configurable solid infill pattern | `solid_fill_pattern` | `d3017889c0` | supermerill, 2019-02-13 |
| Improve bridge region detection (offset2 erosion) | LayerRegion.cpp | `568aa1d113` → `1822af7854` | supermerill, 2019-02-14 / 2021-11-06 |
| Alternate extra perimeter | `extra_perimeters_odd_layers` | `004ec5480d` | supermerill, 2019-10-29 |
| Calculate extrusion width from nozzle | "phony" spacing settings | `715d58da88` | Remi Durand, 2021-04-25 |
| Auto-compute role extrusion widths (fix) | — | `1f41261ad7` (SuperSlicer#4349) | supermerill |

Notes:
- **Solid infill pattern** has Slic3r-era ancestry: `038caddcda` (Alessandro
  Ranellucci, 2011, "different patterns for solid layers" #20). PrusaSlicer
  dropped it, SuperSlicer kept it, BOSS restored it.
- **Apply thick bridge flow to external bridges** has no clean SuperSlicer commit.
  SuperSlicer's `LayerRegion::bridging_flow()` simply has no internal/external
  gate, so external bridges get thick flow inherently. The `thick_bridges` knob
  itself is upstream Prusa (`ceea9de8b8`, Vojtech Bubnik, 2021-03-15), not
  SuperSlicer.

## OrcaSlicer

| BOSS feature | OrcaSlicer commit(s) | Author / date | PR |
|--------------|----------------------|---------------|-----|
| Configurable bridge density | `fe9a2715e6`, `fe6ce5e28a`, `ab5e7f7de8` | SoftFever, 2023-02-02 / 05-22 | #275, #1043 |
| CrossHatch infill | `226450ea6a` (from BambuStudio 1.9 beta4) | SoftFever, 2024-04-28 | #5181 |
| 3D Honeycomb direction switch | `587fab285c`, later `f27605eac1` | David Eccles (gringer), 2024-03-16 | #4425, #12062 |
| Reverse extrusion on odd layers | `951252c597` + `cd475f0f94` (internal-only) | Noisyfox / Ioannis Giannakas, 2023-10/11 | #2413, #2722 |
| Aligned rear seam placement | `8f3ed9bc7b` ("Aligned back") | SoftFever, 2025-07-29 | #10255 |
| Fuzzy skin structured noise | `fd0b2547f2` | Nick Johnson, 2025-01-27 | #7678 |
| Z-hop surface filtering (`retract_lift_enforce`) | `2a478ab4f9` (BambuStudio feature, via BS1.7.4 merge) | SoftFever, 2023-08-26 | — |

Notes:
- **Fuzzy skin structured noise**: BOSS reimplemented with FastNoiseLite (MIT,
  header-only) instead of Orca's noise library. Output differs; quality
  equivalent. BOSS ported Perlin/Billow/Ridged/Voronoi, not Orca's Ripple.
- **Z-hop surface filtering**: the config key matches Orca, but the feature
  originates in BambuStudio and entered Orca via the BS1.7.4 merge commit.
- **Small area flow comp is NOT an OrcaSlicer port** — see Community contributors.

## preFlight

All commits are squashed per-release snapshots by oozeBot R&D.

| BOSS feature | preFlight version | Release commit | Notes |
|--------------|-------------------|----------------|-------|
| Configurable perimeter overlap | v0.9.0 (present at first release) | `ff31cc5205` | BOSS referenced v0.9.4 (`727d4ea7f4`) |
| Nip/Tuck (V-Notch) seam hiding | v0.9.3 (introduced) | `ac7d12ea43` | BOSS upgraded to v0.9.9 base (`1559f778d8`). BOSS 3.0 port (`feature-nip-tuck-seam`, from `boss` `901d98a8c9`): DEVIATION — inner-perimeter trim spacing no longer resolves `external_perimeter_overlap`, since that option now lives on a separate, independently-composable feature branch (`feature-perimeter-overlap`); spacing computes as `ext_width` (equivalent to zero overlap) unless that branch is composed alongside this one |
| Sparse infill absorption | v0.9.7 (origin) | `1c74b8e42d` | BOSS ported at v0.9.9 |
| Detect narrow solid infill (erosion) | v0.9.7 (origin) | `1c74b8e42d` | preFlight later swapped Voronoi medial axis → erosion at v0.9.15 (`c7dbb58d78`); BOSS made the same change independently |
| Paint-on line drawing | v0.9.7 | `1c74b8e42d` | |
| Painted seam alignment | v0.9.7 | `1c74b8e42d` | BOSS added reference_position tracking (deviation) |
| Close gaps solid infill ↔ perimeter | v0.9.9 | `1559f778d8` | |
| Small perimeter speed base resolution | v0.9.9 | `1559f778d8` | |
| Prevent double notch (folded into Nip/Tuck on `boss`) | v0.9.14 | `c6cc822e9f` | DEVIATION: dropped preFlight's interlocking-shell fallback |
| Widen paint-on snap to 15° (folded into paint-on on `boss`) | v1.0.1-beta2 | `99f70c2f9a` | |

## Community contributors

| BOSS feature | Source |
|--------------|--------|
| Small Area Infill Flow Compensation | Script by Alexander Thor @Alexander-T-Moss (v0.7.1, `26d5230`); C++ by Morton Jonuschat. The PrusaSlicer/BOSS C++ implementation came first; the OrcaSlicer port (`82ead12cde`, #3334) is by the same author. The two then evolved in parallel: BOSS switched to Akima interpolation, OrcaSlicer to PCHIP. So Orca is not the source — it is a sibling. |
| Double digit date/time format in filename | Vovodroid @Vovodroid |
| Enable seams in preview tab by default | Iulian Onofrei @revolter |
| Per-feature jerk/SCV/min cruise ratio control | Vovodroid @Vovodroid + Morton Jonuschat |

## BOSS-original (no upstream sync needed)

Flowsnake/Gosper curve infill (L-system on `FillPlanePath`), Akima interpolation
for small-area flow comp (BOSS evolution of the author's own original; Orca went
PCHIP), first layer/top flow ratio, per-object extrusion multiplier,
pre-heating, filament max speed, bed/plate placeholder, wipe tower settings, small
perimeter threshold, external-perimeters-first options, pre-prime extruder, detect
narrow internal solid infill, auto-arrange fix, marker size, merge
SET_VELOCITY_LIMIT, Klipper SCV cornering, git commit hash in builds, plus
assorted bug fixes.
