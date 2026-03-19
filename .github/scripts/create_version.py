#!/usr/bin/env python3

import json
import argparse
import re
import sys

from pathlib import Path
from urllib.request import urlopen


def version_key(v: str) -> tuple[int, ...]:
    """Numeric version comparison within a release kind.

    Returns an empty tuple for empty strings so that any real version
    compares greater (Python tuples compare element-wise, and () < (n,)).

    e.g. '2.10.0-rc2' -> (2, 10, 0, 2)
         '2.9.4'      -> (2, 9, 4, 0)
         ''           -> ()
    """
    if not v:
        return ()
    parts = v.split("-", 1)
    nums = [int(x) for x in parts[0].split(".")]
    nums.append(int(re.sub(r"[^\d]", "", parts[1])) if len(parts) > 1 else 0)
    return tuple(nums)


RELEASES_URL = "https://api.github.com/repos/mjonuschat/PrusaSlicer/releases"
VERSION_PATTERN = re.compile(
    r"version_(?P<version>\d+\.\d+\.\d+)(-(?P<suffix>(alpha|beta|rc)\d+))?\+boss"
)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("output", help="Write version information to this file")
    args = parser.parse_args()

    with urlopen(f"{RELEASES_URL}?per_page=100") as response:
        body = response.read()
        releases = json.loads(body)

    versions: dict[str, str] = {}
    assets: dict[str, dict] = {}

    for release in releases:
        if release.get("draft", False):
            continue
        if not (tag := release.get("tag_name", "")):
            continue
        if not (match := VERSION_PATTERN.fullmatch(tag)):
            continue

        version = match.group("version")
        suffix = match.group("suffix") or ""

        if suffix.startswith("rc"):
            kind = "rc"
            full_version = f"{version}-{suffix}"
        elif suffix.startswith("beta"):
            kind = "beta"
            full_version = f"{version}-{suffix}"
        elif suffix.startswith("alpha"):
            kind = "alpha"
            full_version = f"{version}-{suffix}"
        else:
            kind = "release"
            full_version = version

        if version_key(versions.get(kind, "")) < version_key(full_version):
            versions[kind] = full_version

            if kind == "release":
                for asset in release.get("assets", []):
                    name = asset.get("name", "")
                    if not name:
                        continue

                    if "Linux" in name:
                        assets["linux"] = asset
                    elif "MacOS-universal" in name:
                        assets["osx"] = asset
                    elif "win64" in name:
                        assets["win64"] = asset

    if "release" not in versions:
        print("ERROR: No matching release found on GitHub", file=sys.stderr)
        sys.exit(1)

    contents = []
    if version := versions.get("release"):
        contents.append(version)
    if version := versions.get("alpha"):
        contents.append(f"alpha={version}")
    if version := versions.get("beta"):
        contents.append(f"beta={version}")
    if version := versions.get("rc"):
        contents.append(f"rc={version}")
    contents.append("")

    contents.append("[common]")
    for kind in ["release", "alpha", "beta", "rc"]:
        if version := versions.get(kind, ""):
            contents.append(f"{kind} = {version}")

    contents.append("")
    for platform in ["win64", "linux", "osx"]:
        asset = assets.get(platform)
        if not asset:
            continue

        url = asset.get("browser_download_url", "")
        if not url:
            continue

        size = asset.get("size", 0)

        contents.append(f"[release:{platform}]")
        contents.append(f"url = {url}")
        match platform:
            case "osx":
                contents.append(f"size = {size}")
            case _:
                contents.append("action = browser")
        contents.append("")

    with open(Path(args.output), "w", encoding="utf-8") as f:
        print("\n".join(contents), file=f)


if __name__ == "__main__":
    main()
