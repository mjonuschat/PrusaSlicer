"""Composes and pushes a BOSS 3.0 build/<version> bookmark end to end:
creates a throwaway jj workspace, runs compose.py in it, pushes the
result, and tears the workspace down again -- the sequence release/nightly
publishing otherwise requires running by hand.

Dependency-free by design (stdlib only) -- see generate_boss_features.py
for the same constraint and its rationale.
"""
from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
import uuid
from pathlib import Path
from typing import Callable, Sequence

sys.path.insert(0, str(Path(__file__).resolve().parent))
import compose  # noqa: E402


class PublishError(Exception):
    """Raised when a jj/git step outside compose.py itself fails."""


Runner = Callable[[Sequence[str], Path], str]


def make_subprocess_runner() -> Runner:
    def run(args: Sequence[str], cwd: Path) -> str:
        result = subprocess.run(args, cwd=cwd, capture_output=True, text=True)
        if result.returncode != 0:
            raise PublishError(
                f"command failed ({' '.join(args)}): {result.stderr.strip()}"
            )
        return result.stdout

    return run


def resolve_version(release: str | None) -> str:
    return release if release is not None else "nightly"


def workspace_dir_for(repo: Path, version: str) -> Path:
    return repo / ".worktrees" / f"compose-{version}-{uuid.uuid4().hex[:8]}"


def run_publish(
    run: Runner,
    repo: Path,
    manifest_path: Path,
    base: str,
    version: str,
    keep_workspace: bool = False,
) -> int:
    workspace_dir = workspace_dir_for(repo, version)
    try:
        workspace_dir.parent.mkdir(parents=True, exist_ok=True)
        run(["jj", "workspace", "add", str(workspace_dir), "-r", base], repo)
    except PublishError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    try:
        workspace_run = compose.make_subprocess_runner(workspace_dir)
        result = compose.run_compose(
            workspace_run, manifest_path, base, version, workspace_dir
        )
        if result != 0:
            return result

        run(["jj", "git", "push", "--bookmark", f"build/{version}"], workspace_dir)
        print(f"pushed build/{version}")
        return 0
    except PublishError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    finally:
        if not keep_workspace:
            run(["jj", "workspace", "forget", workspace_dir.name], repo)
            shutil.rmtree(workspace_dir, ignore_errors=True)


def main(argv: Sequence[str] | None = None) -> int:
    default_manifest = Path(__file__).resolve().parent / "manifest.json"
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--release", help="version string for a release build/<version> bookmark; "
        "defaults to a nightly build if omitted"
    )
    parser.add_argument(
        "--manifest", type=Path, default=default_manifest, help="path to manifest.json"
    )
    parser.add_argument(
        "--base", default="foundation", help="bookmark/revision to compose onto"
    )
    parser.add_argument(
        "--repo", type=Path, default=Path("."),
        help="path to the main jj-colocated repo (not a workspace)"
    )
    parser.add_argument(
        "--keep-workspace", action="store_true",
        help="don't remove the throwaway workspace afterward"
    )
    args = parser.parse_args(argv)

    run = make_subprocess_runner()
    return run_publish(
        run, args.repo, args.manifest, args.base,
        resolve_version(args.release), args.keep_workspace,
    )


if __name__ == "__main__":
    sys.exit(main())
