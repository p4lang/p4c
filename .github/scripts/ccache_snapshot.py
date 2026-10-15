#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

"""Save the main cache, or just the files a PR added or changed.

Compare file contents because ccache updates timestamps on cache hits.
Only copy result files (R) and manifest files (M), which ccache uses to find
results. Leave settings, statistics and temporary files out of PR caches.
"""

import argparse
import hashlib
import json
import os
import shutil
import subprocess
from collections.abc import Iterable
from pathlib import Path


def cache_files(directory: Path) -> list[Path]:
    files = []
    for path in directory.rglob("*"):
        if path.is_symlink() or not path.is_file():
            continue
        if path.name.endswith(("R", "M")):
            files.append(path)
    return files


def file_hashes(directory: Path) -> dict[str, str]:
    """Map each cache file's relative path to a hash of its contents."""
    hashes = {}
    for path in cache_files(directory):
        digest = hashlib.sha256()
        with path.open("rb") as stream:
            while chunk := stream.read(1024 * 1024):
                digest.update(chunk)
        name = str(path.relative_to(directory))
        hashes[name] = digest.hexdigest()
    return hashes


def copy_files(source: Path, destination: Path, names: Iterable[str]) -> None:
    for name in names:
        target = destination / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source / name, target)


def save_pr_files(cache: Path, pr_cache: Path, names: list[str], max_bytes: int) -> list[str]:
    """Copy the changed files, keeping newer ones first if space runs out."""
    files = [cache / name for name in names]
    files.sort(key=lambda path: path.stat().st_mtime_ns, reverse=True)
    shutil.rmtree(pr_cache, ignore_errors=True)
    pr_cache.mkdir(parents=True)
    remaining = max_bytes
    selected = []
    for path in files:
        size = path.stat().st_size
        if size > remaining:
            continue
        selected.append(str(path.relative_to(cache)))
        remaining -= size
    copy_files(cache, pr_cache, selected)
    return selected


def prepare_cache(
    cache: Path,
    pr_cache: Path,
    state: Path,
    *,
    event: str,
    ref: str,
    save_main: bool,
    max_pr_bytes: int,
) -> str:
    """Prepare files for upload and return 'main', 'pr', or 'none'."""
    subprocess.run(["ccache", "--cleanup"], check=True)
    subprocess.run(["ccache", "--show-stats"], check=True)
    current = file_hashes(cache)
    before_build = json.loads((state / "before-build.json").read_text())
    changed = any(before_build.get(name) != digest for name, digest in current.items())
    has_results = any(name.endswith("R") for name in current)
    if not changed or not has_results:
        return "none"

    if ref == "refs/heads/main" and save_main:
        return "main"
    if event != "pull_request":
        return "none"

    main_cache = json.loads((state / "main-cache.json").read_text())
    additions = [name for name, digest in current.items() if main_cache.get(name) != digest]
    saved = save_pr_files(cache, pr_cache, additions, max_pr_bytes)
    if any(name.endswith("R") for name in saved):
        return "pr"
    return "none"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("phase", choices=("record-main", "apply-pr", "prepare"))
    args = parser.parse_args()
    cache = Path(".ccache")
    pr_cache = Path(".ccache-pr")
    state = Path(os.environ["RUNNER_TEMP"]) / "p4c-ccache"
    state.mkdir(parents=True, exist_ok=True)

    if args.phase == "record-main":
        (state / "main-cache.json").write_text(json.dumps(file_hashes(cache)))
        return
    if args.phase == "apply-pr":
        names = [str(path.relative_to(pr_cache)) for path in cache_files(pr_cache)]
        copy_files(pr_cache, cache, names)
        (state / "before-build.json").write_text(json.dumps(file_hashes(cache)))
        return

    destination = prepare_cache(
        cache,
        pr_cache,
        state,
        event=os.environ["GITHUB_EVENT_NAME"],
        ref=os.environ["GITHUB_REF"],
        save_main=os.environ.get("CACHE_SAVE_MAIN", "true") == "true",
        max_pr_bytes=int(os.environ.get("CACHE_PR_MAX_BYTES", "200000000")),
    )
    with open(os.environ["GITHUB_OUTPUT"], "a", encoding="utf-8") as output:
        output.write(f"main={str(destination == 'main').lower()}\n")
        output.write(f"pr={str(destination == 'pr').lower()}\n")
    print(f"Cache to upload: {destination}")


if __name__ == "__main__":
    main()
