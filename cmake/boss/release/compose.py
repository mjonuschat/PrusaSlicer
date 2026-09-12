"""Composes BOSS 3.0's foundation branch with a selected set of bugfix and
feature branches into a single build/<version> bookmark, for local testing
or a release build.

Dependency-free by design (stdlib only) -- see generate_boss_features.py
for the same constraint and its rationale.
"""
from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Sequence


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
