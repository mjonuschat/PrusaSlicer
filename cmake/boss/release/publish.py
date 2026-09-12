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


def discard_chain(run: Runner, repo: Path, version: str, target: str) -> None:
    """Drops the composed bookmark and the chain it named, pushed or not."""
    run(["jj", "bookmark", "delete", f"build/{version}"], repo)
    run(["jj", "abandon", "-r", f"::{target} ~ ::bookmarks()"], repo)


def run_cleanup(repo: Path, compose_run: compose.Runner | None = None) -> int:
    """Abandons composed chains left by an interrupted or hand-run compose."""
    if compose_run is None:
        compose_run = compose.make_subprocess_runner(repo)
    try:
        pruned = compose.prune_orphaned_chains(compose_run)
    except compose.ComposeError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    print(
        f"pruned {pruned} commit(s) from orphaned composed chains"
        if pruned else "no orphaned composed chains"
    )
    return 0


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
            if result == compose.ComposeResult.CONFLICT:
                keep_workspace = True
            return result

        target = run(
            ["jj", "log", "--no-graph", "-r", f"build/{version}", "-T", "commit_id"], repo
        ).strip()

        try:
            run(["jj", "git", "push", "--bookmark", f"build/{version}"], workspace_dir)
        except PublishError:
            try:
                discard_chain(run, repo, version, target)
            except PublishError as cleanup_exc:
                print(f"warning: {cleanup_exc}", file=sys.stderr)
            else:
                print(
                    f"build/{version} was not pushed; its composed chain has "
                    "been discarded, so re-run to try again",
                    file=sys.stderr,
                )
            raise
        print(f"pushed build/{version}")

        discard_chain(run, repo, version, target)
        return 0
    except PublishError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    finally:
        if not keep_workspace:
            try:
                run(["jj", "workspace", "forget", workspace_dir.name], repo)
            except PublishError as exc:
                # Never let teardown replace the error that got us here.
                print(f"warning: {exc}", file=sys.stderr)
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
    parser.add_argument(
        "--cleanup", action="store_true",
        help="abandon orphaned composed chains and exit without composing"
    )
    args = parser.parse_args(argv)

    run = make_subprocess_runner()
    if args.cleanup:
        return run_cleanup(args.repo)
    return run_publish(
        run, args.repo, args.manifest, args.base,
        resolve_version(args.release), args.keep_workspace,
    )


if __name__ == "__main__":
    sys.exit(main())
