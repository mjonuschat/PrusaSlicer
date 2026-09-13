# Changelog

Manual changelog for BOSS-specific updates, fixes, and porting notes.

This file is intentionally maintained by hand so release-relevant context is not
lost during the rebase and squash based workflow.

## Unreleased

### Added

### Changed

### Fixed

### Ported

Every entry below already shipped in BOSS 2.9.x. These are listed as
ported to the 3.0 architecture, not as new work.

#### Infill

- Small-area Infill Flow Compensation: reduces flow on short solid-infill
  segments to avoid over-extrusion and rough surfaces.
- Configurable Bridge Density: bridge fill density from 10–120%,
  improving bridging quality and visual appearance.
- First/Top Layer Flow Ratio: independent flow control for the first
  layer and top surface, fixing poor bed adhesion or a rough top.
- CrossHatch Infill Pattern: a faster, quieter alternative to Gyroid
  that maintains strength and avoids nozzle collisions on large grid
  infills.
- Flowsnake Infill Pattern: Gosper-curve-based infill with a visually
  distinctive top/bottom surface.
- Structured Fuzzy Skin: structured noise (Perlin, Billow, Ridged
  Multifractal, Voronoi) for a more natural-looking textured surface
  compared to uniform jitter.
- Internal Solid-Fill Pattern: sets the internal solid-infill type
  independent of the top/bottom pattern, for finer control over
  internal strength.
- Narrow Solid-Infill Erosion Detection: switches narrow solid areas
  to Arachne's variable-width fill, avoiding zigzag artifacts and gaps.
- Sparse Infill Absorption: merges small sparse-infill pockets enclosed
  by solid infill, avoiding tiny, ineffective patches.

#### Perimeters / walls

- Alternate Extra Perimeter: adds one extra wall every other layer,
  for stronger prints, with fill and walls interlocking.
- External Perimeters First for Holes: controls if a hole's outer wall
  prints first, for better print quality and more accurate holes.
- Configurable Perimeter Overlap: adjusts overlap between adjacent
  walls, for finer control over wall bonding and thickness.
- Configurable Small Perimeter Threshold: controls the length below
  which the small-perimeter speed applies to a perimeter.
- Reverse Extrusion Direction on Odd Layers: alternates perimeter/infill
  direction, reducing warping and stress buildup.

#### Seams

- Aligned-Rear Seam Placement: biases the aligned seam mode toward the
  back of the model, to hide seams from view.
- Painted Seam Alignment/Blending: keeps the seam inside the area you
  paint, even across layers, instead of drifting outside it.
- Nip/Tuck (V-Notch) Seam Hiding: cuts a small notch at the seam,
  hiding start/stop blobs for less visible seams.
- Seam Visible in Preview by Default: shows seam markers in the G-code
  preview by default, so placement can be checked without an extra
  setting change.

#### Wipe tower / toolchange

- Disable Wipe Tower Ramming/Cooling: independent switches for ramming
  and cooling moves, allowing more granular control for filament
  changer setups.
- Wipe Tower Maximum Purge Speed: caps purge move speed for more
  reliable purging in tall wipe towers.
- Force Wipe Tower Linear Advance Suppression: prevents pressure
  advance from being silently disabled for the rest of the print,
  PrusaSlicer's default behavior at purge points on Klipper.
- Disable Automatic Tool-Change Commands: turns off automatic emission
  of Tx commands, allowing for fully customized toolchange G-code.
- Prime Length at Start: primes the first extruder before printing
  begins, avoiding small gaps at the start of the first layer.
- Toolchange Preheat: every multi-tool printer gets it, not only the Prusa
  XL, so a tool change no longer waits for the new tool to reach
  temperature. Printers that do not understand the Prusa M104.1 command get
  a plain temperature command, and their ooze prevention cooldowns stay in
  place. Set the preheat time to 0 to turn it off.

#### Motion / Klipper

- Per-Feature Jerk, SCV, and Minimum Cruise Ratio: sets jerk, cornering
  speed, and minimum cruise ratio separately for perimeters, infill,
  bridges, and other print roles.
- Klipper Print Time Estimation: estimates print time using Klipper's
  own cornering math, instead of a generic approximation.
- Merge Klipper Velocity-Limit Commands: merges consecutive
  SET_VELOCITY_LIMIT lines, avoiding redundant commands in the G-code.
- Exclude Object for Skirt/Brim/Wipe Tower: reports skirt, brim, and
  wipe tower to Klipper without making them cancelable, so the print's
  size and bounding box stay accurate.
- Z-Hop Surface Filtering: restricts Z-hop to specific surfaces,
  improving print time while avoiding the stringing, blobs, or surface
  blemishes that disabling Z-hop everywhere would cause.

#### Filament / extrusion config

- Filament Maximum Speed: caps print speed per filament, avoiding
  speeds that the material cannot handle.
- Per-Object Extrusion Multiplier: sets flow per object, for
  fine-tuning a single part without editing the filament profile.
- Expanded Filament Type List: adds more specific filament types,
  matching the wide range of modern filament materials.

#### Misc

- Bed Number Placeholder: adds a `bed_number` filename placeholder,
  for organizing output across multi-bed print jobs.
- Zero-Padded Date/Time Placeholders: zero-pads filename values,
  keeping generated filenames sorting correctly.
- Z-Rotate on Import: rotates imported models automatically on the Z
  axis, avoiding a manual reorientation step.

#### Bugfixes

- 3D Honeycomb Infill Bridge Direction: fixes incorrect bridge
  geometry and direction over open spans.
- Arachne Duplicate Thin Wall Segments: fixes duplicate overlapping
  wall segments near the 1-to-2 bead transition.
- Auto-Arrange Nesting: restores nesting of small objects inside the
  holes of larger ones during auto-arrange.
- Modifier Bridge/Infill Speeds: fixes fill grouping so modifier
  volumes respect both bridge- and infill-speed overrides.
- Solid Infill/Perimeter Gap: fixes a gap between solid infill and the
  perimeter that weakened top layers.
- Unique Labeled Object Names: fixes duplicate labels for same-named
  objects on Klipper, which could let Cancel Object target the wrong
  one.
- Wipe Tower Divide-by-Zero: fixes invalid speed values in the G-code
  caused by dividing by a zero loading distance or speed.

### Notes

## 2.9.6 - 2026-06-25

### Fixed

- Modifier volumes that override bridge or infill speeds now keep their own
  fill groups, so bridge-speed overrides are respected in generated G-code.
- Arachne no longer generates duplicate overlapping wall segments for thin
  frames near the 1-to-2 bead transition.

## 2.9.6-rc1 - 2026-06-17

### Changed

- Paint-on line drawing now snaps to vertical within 15 degrees instead of 5,
  making the vertical snap easier to trigger on curved surfaces.

### Fixed

- Automatic role-specific extrusion widths now compute their default values when
  both the role width and default extrusion width are set to auto, instead of
  resolving to zero.
- Nip/Tuck seams no longer create a doubled notch on thin walls where two outer
  perimeters share a single inner perimeter. The shared inner is now split so
  each outer perimeter gets its own clean notch gap.
- 3D Honeycomb infill is now consistent between layers when "combine infill
  every N layers" is enabled. Previously the pattern drifted with the combined
  layer height and produced poor bridges.
- Fixed ooze-prevention preheat commands for first-layer tool changes. Tools
  first used after the initial tool on layer one now preheat to their
  first-layer nozzle temperature instead of their normal layer temperature.
