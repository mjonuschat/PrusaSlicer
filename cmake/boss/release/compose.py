"""Composes BOSS 3.0's foundation branch with a selected set of bugfix and
feature branches into a single build/<version> bookmark, for local testing
or a release build.

Dependency-free by design (stdlib only) -- see generate_boss_features.py
for the same constraint and its rationale.
"""
from __future__ import annotations

import argparse
import json
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Sequence

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import generate_boss_features  # noqa: E402


class ManifestError(Exception):
    """Raised when manifest.json is malformed."""


@dataclass(frozen=True)
class ExcludedEntry:
    branch: str
    reason: str


def load_manifest(path: Path) -> list[ExcludedEntry]:
    try:
        raw_text = path.read_text(encoding="utf-8")
    except FileNotFoundError:
        return []

    try:
        data = json.loads(raw_text)
    except json.JSONDecodeError as exc:
        raise ManifestError(f"{path}: invalid JSON: {exc}") from exc

    if not isinstance(data, dict) or set(data.keys()) - {"excluded"}:
        raise ManifestError(f"{path}: top-level object must contain only 'excluded'")

    excluded_raw = data.get("excluded", [])
    if not isinstance(excluded_raw, list):
        raise ManifestError(f"{path}: 'excluded' must be a list")

    entries: list[ExcludedEntry] = []
    seen: set[str] = set()
    for item in excluded_raw:
        if not isinstance(item, dict) or set(item.keys()) != {"branch", "reason"}:
            raise ManifestError(
                f"{path}: each excluded entry must have exactly 'branch' and 'reason'"
            )
        branch, reason = item["branch"], item["reason"]
        if not isinstance(branch, str) or not branch:
            raise ManifestError(f"{path}: 'branch' must be a non-empty string")
        if not isinstance(reason, str) or not reason:
            raise ManifestError(f"{path}: 'reason' must be a non-empty string")
        if branch in seen:
            raise ManifestError(f"{path}: duplicate excluded branch {branch!r}")
        seen.add(branch)
        entries.append(ExcludedEntry(branch=branch, reason=reason))
    return entries


BUGFIX_PREFIX = "bugfix-"
FEATURE_PREFIX = "feature-"


class ComposeError(Exception):
    """Raised when composition cannot proceed."""


@dataclass(frozen=True)
class ComposeSet:
    bugfixes: list[str]
    features: list[str]

    def ordered(self) -> list[str]:
        return [*self.bugfixes, *self.features]


def resolve_branch_set(
    all_branches: Sequence[str], excluded: Sequence[ExcludedEntry]
) -> ComposeSet:
    excluded_names = {entry.branch for entry in excluded}
    stale = excluded_names - set(all_branches)
    if stale:
        raise ComposeError(
            "manifest excludes branch(es) that no longer exist: "
            + ", ".join(sorted(stale))
        )
    included = [b for b in all_branches if b not in excluded_names]
    bugfixes = sorted(b for b in included if b.startswith(BUGFIX_PREFIX))
    features = sorted(b for b in included if b.startswith(FEATURE_PREFIX))
    return ComposeSet(bugfixes=bugfixes, features=features)


Runner = Callable[[Sequence[str]], str]


def list_all_branches(run: Runner) -> list[str]:
    output = run(["jj", "bookmark", "list"])
    names: list[str] = []
    for line in output.splitlines():
        line = line.strip()
        if not line or ":" not in line:
            continue
        name = line.split(":", 1)[0].strip()
        if name.startswith(BUGFIX_PREFIX) or name.startswith(FEATURE_PREFIX):
            names.append(name)
    return names


def is_conflicted(run: Runner, revision: str = "@") -> bool:
    output = run(["jj", "log", "--no-graph", "-r", revision, "-T", "conflict"]).strip()
    if output not in ("true", "false"):
        raise ComposeError(f"unexpected conflict-check output: {output!r}")
    return output == "true"


def compose_branches(
    run: Runner, base: str, branch_set: ComposeSet, description: str
) -> None:
    run(["jj", "new", base])
    run(["jj", "describe", "-r", "@", "-m", description])
    for branch in branch_set.ordered():
        run(["jj", "new", "@", branch])
        run(["jj", "describe", "-r", "@", "-m", description])
        if is_conflicted(run):
            raise ComposeError(
                f"composition conflict merging {branch!r}; "
                "workspace left as-is for manual resolution"
            )


def make_subprocess_runner(cwd: Path) -> Runner:
    def run(args: Sequence[str]) -> str:
        result = subprocess.run(args, cwd=cwd, capture_output=True, text=True)
        if result.returncode != 0:
            raise ComposeError(
                f"command failed ({' '.join(args)}): {result.stderr.strip()}"
            )
        return result.stdout

    return run


def validate_boss_features(repo: Path) -> None:
    features_dir = repo / "src" / "boss" / "include" / "boss" / "features"
    with tempfile.TemporaryDirectory() as scratch:
        if generate_boss_features.generate(features_dir, Path(scratch)) != 0:
            raise ComposeError(
                "boss feature generation failed for the composed tree (see above)"
            )


def run_compose(run: Runner, manifest_path: Path, base: str, version: str, repo: Path) -> int:
    try:
        if not manifest_path.exists():
            raise ManifestError(f"manifest not found: {manifest_path}")

        excluded = load_manifest(manifest_path)
        all_branches = list_all_branches(run)
        branch_set = resolve_branch_set(all_branches, excluded)

        composed = branch_set.ordered()
        branches_desc = ", ".join(composed) if composed else "no additional branches"
        bookmark = f"build/{version}"
        description = f"{bookmark}: compose {branches_desc}"
        compose_branches(run, base, branch_set, description)

        validate_boss_features(repo)

        run(["jj", "bookmark", "create", bookmark, "-r", "@"])

        print(f"composed {len(composed)} branch(es) onto {bookmark}")
        print(f"push with: jj git push --bookmark {bookmark}")
        return 0
    except (ManifestError, ComposeError, FileNotFoundError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description=__doc__,
        epilog=(
            "Run this from a throwaway `jj workspace add` directory, not your "
            "main workspace. Composition moves @ through a series of new "
            "commits and ends by creating a bookmark, which displaces "
            "whatever you were working on if run in place."
        ),
    )
    parser.add_argument(
        "--manifest", type=Path, required=True, help="path to manifest.json"
    )
    parser.add_argument(
        "--base", default="foundation", help="bookmark/revision to compose onto"
    )
    parser.add_argument(
        "--version", required=True, help="version string for the build/<version> bookmark"
    )
    parser.add_argument(
        "--repo", type=Path, default=Path("."), help="path to the jj-colocated repo"
    )
    args = parser.parse_args(argv)

    run = make_subprocess_runner(args.repo)
    return run_compose(run, args.manifest, args.base, args.version, args.repo)


if __name__ == "__main__":
    sys.exit(main())
