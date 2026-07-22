# Preset Updater — Manual Test Checklist

A hands-on checklist for verifying the Preset Updater end-to-end against the **live
production** preset-repo-api. Follow it while actually using the built application — it is
not automated. It complements the automated tests in
`src/slic3r-shared/test/Slic3r/Biz/PresetUpdater/` (which run entirely against a mock
server), by exercising the real network path, the real server contents, and the real files
written to disk.

## Why this exists

The endpoint was switched to production (`https://preset-repo-api.prusa3d.com`, see
`Slic3r::Biz::Network::ServiceConfig`). Automated tests deliberately never touch the network,
so a human pass is the only thing that confirms the production server + full download/install
pipeline actually work together.

## Setup

1. Build a normal binary (GUI + launcher).
2. **Always use a throwaway data directory** so your real profiles are never touched. Pass
   `--datadir <tmp>` on every command below, pointing at an empty scratch folder, e.g.
   `--datadir C:\tmp\pu_manual`.
3. Endpoint selection:
   - Default is production. No action needed to test production.
   - To point at a staging/dev server instead, set `PRUSA_PRESET_REPO_URL` (only URLs on
     whitelisted Prusa domains are accepted — see `ServiceConfig::update_url_from_env`).
4. The reliable live surface today is the **CLI actions** below. The in-app GUI updater
   (`Slic3r::App::PresetUpdaterUI`) is still under construction — automatic sync on startup is
   commented out and reconfigurations are shown via a raw debug yes/no dialog — so drive the
   flow from the command line and inspect the datadir on disk.

Locations inside `<datadir>` to inspect:

| Path | Meaning |
| --- | --- |
| `shared_runtime/RepositoryManifest.json` | The list of sources and their `selected` flags |
| `update_sync/<repo_id>/...` | Files staged (downloaded) but not yet installed |
| `presets/local/<repo_id>/<Vendor>/` + `<Vendor>.idx` | Installed vendor profiles |
| `local_repositories/<uuid>/` | Unzipped local (offline) repositories |

## Checklist

Tick each step. On failure, record the exact command, the JSON printed, and the relevant
tree under `<datadir>` (see "On failure" below).

### 1. List sources
```
prusa-slicer --datadir <tmp> --preset-update-sources
```
- [ ] Prints a JSON array of sources fetched from production.
- [ ] The Prusa production repository is present and `selected: true`.
- [ ] `shared_runtime/RepositoryManifest.json` now exists and matches the printed list.

### 2. Sync / list reconfigurations (clean datadir)
```
prusa-slicer --datadir <tmp> --preset-update-list
```
- [ ] Returns JSON with the reconfigurations (on a clean datadir, the online vendors show up
      as `new_vendors`).
- [ ] Downloaded indices/bundles appear under `update_sync/<repo_id>/`.
- [ ] Running the same command again returns an empty/`None` result (nothing new to stage).

### 3. Perform update (download + install)
```
prusa-slicer --datadir <tmp> --preset-update
```
- [ ] Completes without an `error` in the output.
- [ ] Installed profiles appear under `presets/local/<repo_id>/<Vendor>/` with a
      `vendor.yaml`, and an index `presets/local/<repo_id>/<Vendor>.idx`.
- [ ] The installed `vendor.yaml` `version` equals the recommended version from the index.
- [ ] A follow-up `--preset-update-list` now reports `None` (everything up to date).

### 4. Select / deselect a source
```
prusa-slicer --datadir <tmp> --preset-update-sources          # note a source uuid
prusa-slicer --datadir <tmp> --preset-update-select-source <uuid>
```
- [ ] The `selected` flag for that `uuid` toggles in `shared_runtime/RepositoryManifest.json`.
- [ ] A subsequent `--preset-update-list` respects the selection (deselected sources are not
      synced).
- [ ] Selecting a source that shares an `id` with an already-selected one deselects the other
      (only one source per `id` stays selected).

### 5. Add a local (offline) repository
```
prusa-slicer --datadir <tmp> --preset-update-add-local <path-to-repo.zip>
```
- [ ] The local source appears in `--preset-update-sources`, reported as `local`.
- [ ] Its unzipped data appears under `local_repositories/<uuid>/`.
- [ ] `--preset-update-list` then `--preset-update` install profiles from the local source.

### 6. Remove the local repository
```
prusa-slicer --datadir <tmp> --preset-update-remove-local <uuid>
```
- [ ] The source disappears from `--preset-update-sources`.
- [ ] `local_repositories/<uuid>/` is deleted.

### 7. Cleanup staged files
```
prusa-slicer --datadir <tmp> --preset-update-cleanup
```
- [ ] Everything staged under `update_sync/` is removed.
- [ ] Already-installed profiles under `presets/local/` are left untouched.

### 8. In-app flow (best effort)
- [ ] Launch the GUI build against the same `<datadir>` and confirm the updater dialog(s)
      appear and behave. Note: the GUI path is still a work in progress; treat unexpected
      debug dialogs as known limitations, not failures.

## Pass / fail criteria

- **Pass**: steps 1–7 each produce the expected on-disk result and no `error` field in the
  JSON output; step 8 shows no crash.
- **Fail**: any `error` in the output, a missing/incorrect file on disk, or a crash.

## On failure — what to attach

- The exact command line (including `--datadir`).
- The full JSON printed to stdout.
- The relevant subtree of `<datadir>` (`update_sync/`, `presets/local/`,
  `shared_runtime/RepositoryManifest.json`).
- Relevant log lines, especially the `PRESET UPDATER STATUS: target/attempt/delay` lines
  emitted during downloads.
