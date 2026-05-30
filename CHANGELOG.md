# Changelog

Manual changelog for BOSS-specific updates, fixes, and porting notes.

This file is intentionally maintained by hand so release-relevant context is not
lost during the rebase and squash based workflow.

## Unreleased

### Added

### Changed

- Paint-on line drawing now snaps to vertical within 15 degrees instead of 5,
  making the vertical snap easier to trigger on curved surfaces.

### Fixed

- Nip/Tuck seams no longer create a doubled notch on thin walls where two outer
  perimeters share a single inner perimeter. The shared inner is now split so
  each outer perimeter gets its own clean notch gap.
- Fixed ooze-prevention preheat commands for first-layer tool changes. Tools
  first used after the initial tool on layer one now preheat to their
  first-layer nozzle temperature instead of their normal layer temperature.

### Ported

### Notes
