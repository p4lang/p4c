#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
# SPDX-License-Identifier: Apache-2.0

"""Run clang-tidy in parallel using real compilation commands.

PR mode analyzes whole translation units, but fails only on diagnostics in
changed lines. Header/IR/parser changes conservatively select every translation
unit in the configured build. A timeout or an analysis error always fails.
"""

import argparse
import concurrent.futures
import json
import os
import re
import subprocess
import sys
import time
from pathlib import Path

SOURCE_ROOT = Path(__file__).resolve().parent.parent
SOURCES = {".c", ".cc", ".cpp", ".cxx"}
HEADERS = {".h", ".hh", ".hpp", ".hxx", ".def", ".l", ".ll", ".ypp", ".proto"}
EXCLUDED = (
    "backends/ebpf/runtime/",
    "backends/ubpf/runtime/",
    "backends/tc/runtime/",
    "backends/tofino/third_party/",
    "test/frameworks/",
)
PROJECT_DIRS = {
    "backends",
    "control-plane",
    "frontends",
    "ir",
    "lib",
    "midend",
    "test",
    "tools",
    "extensions",
}


def project_file(path):
    """Return an owned source path, excluding runtime fixtures and dependencies."""
    try:
        relative = path.resolve().relative_to(SOURCE_ROOT).as_posix()
    except ValueError:
        return None
    if relative.startswith(EXCLUDED):
        return None
    if relative.split("/", 1)[0] not in PROJECT_DIRS:
        return None
    return relative


def git(*arguments):
    return subprocess.check_output(["git", *arguments], cwd=SOURCE_ROOT).decode()


def changed_lines(base, staged=False):
    revision = ["--cached"] if staged else [f"{base}...HEAD"]
    paths = git("diff", "--name-only", "-z", "--diff-filter=ACDMRT", *revision).split("\0")
    changes = {}
    for name in filter(None, paths):
        path = SOURCE_ROOT / name
        if path.suffix not in SOURCES | HEADERS or project_file(path) is None:
            continue
        diff = git("diff", "--no-ext-diff", "--unified=0", *revision, "--", name)
        if path.suffix in SOURCES and re.search(r"^deleted file mode ", diff, re.MULTILINE):
            continue
        ranges = []
        for start, count in re.findall(r"^@@ .* \+(\d+)(?:,(\d+))? @@", diff, re.MULTILINE):
            start, count = int(start), int(count) if count else 1
            if count:
                ranges.append([start, start + count - 1])
        # Deletion-only edits still require analysis, but have no new lines.
        changes[name] = ranges
    return changes


def translation_units(build, files=None):
    database = json.loads((build / "compile_commands.json").read_text())
    units = set()
    for entry in database:
        path = (Path(entry["directory"]) / entry["file"]).resolve()
        try:
            build_path = path.relative_to(build)
        except ValueError:
            build_path = None
        if (
            build_path
            and "_deps" not in build_path.parts
            and ("Unity" in build_path.parts or path.name.startswith("unified_"))
        ):
            raise ValueError("Unity compilation commands found. Configure a non-unity build.")
        relative = project_file(path)
        if relative and path.suffix in SOURCES and (files is None or path in files):
            units.add(relative)
        elif path.suffix in SOURCES:
            try:
                generated = path.relative_to(build)
            except ValueError:
                continue
            # Include project-generated parsers and IR implementations so #line
            # diagnostics in edited grammars/.def files are checked as well.
            # Never analyze dependency sources or unity amalgamations.
            if (
                generated.parts[0] in PROJECT_DIRS
                and "CMakeFiles" not in generated.parts
                and not generated.name.startswith("unified_")
            ):
                units.add(str(path))
    if not units:
        raise ValueError("No project translation units found. Configure a non-unity build.")
    return sorted(units)


def select_units(units, changes, prefix, exclude_prefix, shard, shards, build=None):
    if changes is not None and not any(Path(name).suffix in HEADERS for name in changes):
        units = [name for name in units if name in changes]

    def in_scope(name):
        if build and Path(name).is_absolute():
            name = Path(name).relative_to(build).as_posix()
        return name.startswith(prefix) and (
            not exclude_prefix or not name.startswith(exclude_prefix)
        )

    units = [name for name in units if in_scope(name)]
    return units[shard::shards]


def analyze(name, args, line_filter):
    started = time.monotonic()
    command = [
        args.clang_tidy,
        "-p",
        str(args.build_dir),
        f"--header-filter=^{re.escape(str(SOURCE_ROOT))}/",
        "--quiet",
        "--use-color=false",
    ]
    if args.base or args.staged:
        command += [f"--line-filter={line_filter}", "--warnings-as-errors=*"]
    if args.profile:
        command += ["--enable-check-profile"]
    if args.fix:
        command += ["--fix"]
    command.append(str(SOURCE_ROOT / name))
    try:
        result = subprocess.run(command, capture_output=True, text=True, timeout=args.timeout)
        code, output = result.returncode, result.stdout + result.stderr
    except subprocess.TimeoutExpired:
        code, output = 124, f"Analysis exceeded {args.timeout}s; coverage is incomplete.\n"
    except OSError as error:
        code, output = 1, str(error)
    return {
        "file": name,
        "seconds": time.monotonic() - started,
        "returncode": code,
        "output": output,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=SOURCE_ROOT / "build")
    parser.add_argument("--clang-tidy", default="clang-tidy")
    parser.add_argument("--file-list", type=Path, help="CMake's semicolon-separated lint file list")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--base", help="Analyze changes since the merge base with this revision")
    mode.add_argument(
        "--staged", action="store_true", help="Check staged changes in the working tree"
    )
    parser.add_argument(
        "--check-changes", action="store_true", help="Print whether C++ files changed"
    )
    parser.add_argument(
        "--prefix",
        action="append",
        default=[],
        help="Limit analysis to these source directories (repeatable)",
    )
    parser.add_argument(
        "--exclude-prefix",
        action="append",
        default=[],
        help="Exclude these source directories (repeatable)",
    )
    parser.add_argument("--shard", type=int, default=0)
    parser.add_argument("--shards", type=int, default=1)
    parser.add_argument("-j", "--jobs", type=int, default=min(os.cpu_count() or 1, 4))
    parser.add_argument(
        "--timeout", type=int, default=3600, help="Seconds allowed per translation unit"
    )
    parser.add_argument("--profile", action="store_true", help="Include per-check timing profiles")
    parser.add_argument("--fix", action="store_true")
    parser.add_argument("--report", type=Path, help="Write timings and exit statuses as JSON")
    args = parser.parse_args()
    args.prefix = tuple(args.prefix) or ("",)
    args.exclude_prefix = tuple(args.exclude_prefix)
    if not 0 <= args.shard < args.shards or args.jobs < 1 or args.timeout < 1:
        parser.error("Invalid shard, job count or timeout")
    if args.fix and (args.jobs != 1 or args.shards != 1):
        parser.error("Fixes must run with -j 1 and a single shard to avoid concurrent header edits")
    changes = changed_lines(args.base, args.staged) if args.base or args.staged else None
    if args.check_changes:
        if changes is None:
            parser.error("--check-changes requires --base or --staged")
        relevant = any(Path(name).suffix in HEADERS for name in changes) or bool(
            select_units(
                sorted(changes), changes, args.prefix, args.exclude_prefix, args.shard, args.shards
            )
        )
        print("true" if relevant else "false")
        return 0
    if changes == {}:
        print("No changed C++ files.")
        return 0
    if args.staged:
        unstaged = {
            name
            for name in git("diff", "--name-only", "-z").split("\0")
            if Path(name).suffix in SOURCES | HEADERS
            and project_file(SOURCE_ROOT / name) is not None
        }
        if unstaged:
            parser.error(
                "Stage or stash unstaged C++ edits before analyzing: " + ", ".join(unstaged)
            )
    args.build_dir = args.build_dir.resolve()
    files = None
    if args.file_list:
        files = {Path(name).resolve() for name in args.file_list.read_text().split(";") if name}
    units = translation_units(args.build_dir, files)
    if changes is not None:
        missing = [
            name
            for name in changes
            if Path(name).suffix in SOURCES
            and name not in units
            and name.startswith(args.prefix)
            and (not args.exclude_prefix or not name.startswith(args.exclude_prefix))
        ]
        if missing:
            parser.error("Changed sources missing compilation commands: " + ", ".join(missing))
    selected = select_units(
        units, changes, args.prefix, args.exclude_prefix, args.shard, args.shards, args.build_dir
    )
    # An empty filter means "all lines" to clang-tidy. A non-existent filename
    # excludes warnings for deletion-only changes; hard analysis errors remain.
    line_filter = json.dumps(
        [
            {"name": str(SOURCE_ROOT / name), "lines": ranges}
            for name, ranges in (changes or {}).items()
            if ranges
        ]
        or [{"name": str(SOURCE_ROOT / ".clang-tidy-no-changed-lines")}]
    )
    print(
        f"Analyzing {len(selected)} translation units with {args.jobs} workers "
        f"(shard {args.shard + 1}/{args.shards}).",
        flush=True,
    )
    results = []
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as executor:
        futures = [executor.submit(analyze, name, args, line_filter) for name in selected]
        for future in concurrent.futures.as_completed(futures):
            result = future.result()
            print(
                f"[{len(results) + 1}/{len(selected)}] {result['file']}: "
                f"{result['seconds']:.1f}s, exit {result['returncode']}",
                flush=True,
            )
            if result["output"]:
                print(result["output"], flush=True)
            results.append(result)
            if args.report:
                args.report.write_text(json.dumps(results, indent=2) + "\n")
    failures = sum(result["returncode"] != 0 for result in results)
    print(f"Analyzed {len(results)} translation units; {failures} failed.")
    return int(failures != 0)


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        sys.exit(str(error))
