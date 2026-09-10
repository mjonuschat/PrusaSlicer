# Changelog

Manual changelog for BOSS-specific updates, fixes, and porting notes.

This file is intentionally maintained by hand so release-relevant context is not
lost during the rebase and squash based workflow.

## Unreleased

### Added

- External bridges can now use a configurable density, from 10% to 120%,
  instead of always printing solid. Higher densities can smooth the
  bridge surface. The default also changes. Bridge fill no longer evens
  out line spacing for full coverage at 100% density, so bridge spacing
  can differ slightly from stock PrusaSlicer even when you do not change
  the setting.
- The first layer and the top solid layer can now each use their own flow
  ratio, from 0.5 to 1.5. Lower the first layer ratio to fix a rough or
  sticking first layer, or raise the top layer ratio to smooth the top
  surface.
- Skirt/brim and the wipe tower are now declared to Klipper hosts
  (Mainsail/Fluidd) via `EXCLUDE_OBJECT_DEFINE`, so their outlines show up
  in the object list, but their G-code is never wrapped as excludable, so
  "Cancel Object" can never remove them and break the print.
- The filament type list now includes more material presets, so a profile
  can name the exact material you print with.
- Output filenames can now use the `bed_number` G-code placeholder, so
  multi-bed print jobs can be named and organized automatically.
- Each object can now set its own extrusion multiplier, so you can adjust
  flow for a single part without changing the filament profile.
- Each filament profile can now set a maximum print speed, so the printer
  never exceeds a speed that filament cannot handle.
- Imported models can now apply a configurable Z-axis rotation
  automatically, so parts that always need reorienting load already
  turned the right way.
- Added the CrossHatch infill pattern, which alternates line direction
  between layers for stronger, quieter prints at high speed.
- Added the Flowsnake infill pattern, with its own bridging-angle
  handling for cleaner bridges over the pattern.
- The wipe tower can now disable filament ramming, cooling moves, or
  both, so a tuned setup is not overridden by the default purge behavior.
- The wipe tower now supports a configurable maximum purge speed, so
  purge moves stay within a speed the setup can handle.
- Linear advance can now be force-disabled at wipe tower purge points, so
  purge extrusion does not fight pressure advance tuning.
- Automatic toolchange command emission can now be disabled, so custom
  toolchange G-code is not duplicated by the automatic sequence.
- The first extruder can now prime at the start of the print, so filament
  is flowing before the first layer begins.
- All toolchangers can now be force-preheated together, so every tool is
  ready before the print reaches its first toolchange.
- Added an alternate extra perimeter option, which adds one extra wall
  every other layer for stronger prints with fill.
- Added an "External perimeters first for holes" option, so holes can
  print their outer wall first independently of contours, with a minimum
  hole size and a first-layers disable so it does not affect fillets or
  chamfers.
- Added configurable perimeter overlap. Two new options,
  "Ext. perimeter/perimeter overlap" and "Perimeter/perimeter overlap",
  control how much adjacent walls overlap, so you can tune wall bonding
  and total wall thickness independently of extrusion width.
- Added a configurable small perimeter speed threshold. Two new options,
  "Lower" and "Upper" small perimeter length, set the length range over
  which a perimeter's speed ramps between the small perimeter speed and
  the normal perimeter speed, instead of switching at a fixed length.
- Added three options to reverse extrusion direction on odd layers, for
  internal perimeters, overhang perimeters, and infill separately, to
  reduce stress and warping and improve steep overhangs.
- Added narrow solid-infill erosion detection. When enabled, narrow
  internal solid infill areas switch to Arachne's variable-width fill
  instead of the configured pattern, so thin solid regions do not print
  with zigzag artifacts or gaps.
- Added a "Solid fill pattern" option, which sets the pattern for internal
  solid infill separately from the top and bottom fill patterns. The
  default is Monotonic.

### Changed

- The `{month}`, `{day}`, `{hour}`, `{minute}`, and `{second}` G-code
  placeholders now zero-pad single-digit values, so generated filenames
  and timestamps sort and line up correctly.
- Seam moves now show in the G-code preview by default, so you can
  review seam placement without changing a setting first.

### Fixed

- Flowsnake infill now uses wider line spacing and a thinner bead, so the
  pattern reads as a visible lattice instead of a dense, squished blob.
- 3D Honeycomb infill now bridges with the correct geometry and
  direction, fixing gaps and misaligned bridges over open spans. (#24)
- Small sparse infill pockets fully enclosed by solid infill are now
  absorbed into the solid fill instead of printing their own sparse
  pattern too small to be useful, small holes in solid infill too narrow
  for any fill to cover are now removed, and solid regions split into
  fragments by bridge angle are now consolidated into one region.

### Ported

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
