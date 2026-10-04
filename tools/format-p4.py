#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
# SPDX-License-Identifier: Apache-2.0

"""Check or update P4 formatting, recursively discovering *.p4 in directories."""

import argparse
import concurrent.futures
import difflib
import os
import pathlib
import stat
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parent.parent


def format_file(formatter, path, timeout, fail_on_fallback):
    original = path.read_bytes()
    command = [formatter]
    if fail_on_fallback:
        command.append("--fail-on-fallback")
    result = subprocess.run([*command, path], capture_output=True, timeout=timeout)
    if result.returncode or result.stderr:
        raise RuntimeError(
            f"formatter exited {result.returncode}: "
            f"{result.stderr.decode(errors='replace').strip()}"
        )
    if original == result.stdout:
        return None
    return path, original, result.stdout


def print_diff(path, original, formatted):
    name = os.fsencode(os.path.relpath(path))
    for line in difflib.diff_bytes(
        difflib.unified_diff,
        original.splitlines(keepends=True),
        formatted.splitlines(keepends=True),
        fromfile=b"a/" + name,
        tofile=b"b/" + name,
    ):
        sys.stdout.buffer.write(line)
        if not line.endswith(b"\n"):
            sys.stdout.buffer.write(b"\n\\ No newline at end of file\n")


def replace_file(path, original, formatted):
    if path.read_bytes() != original:
        raise RuntimeError(f"{path}: file changed while formatting; refusing to overwrite it")
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(dir=path.parent, prefix=".p4fmt-", delete=False) as output:
            temporary = pathlib.Path(output.name)
            output.write(formatted)
        temporary.chmod(stat.S_IMODE(path.stat().st_mode))
        temporary.replace(path)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument(
        "--check", action="store_true", help="print diffs and fail on changes (default)"
    )
    mode.add_argument("--write", action="store_true", help="apply formatting in place")
    parser.add_argument("--formatter", type=pathlib.Path, default=ROOT / "build" / "p4fmt")
    parser.add_argument(
        "--fail-on-fallback",
        action="store_true",
        help="reject unsupported files instead of accepting unchanged fallback output",
    )
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--timeout", type=float, default=20)
    parser.add_argument("paths", type=pathlib.Path, nargs="*", default=[ROOT / "testdata"])
    args = parser.parse_args()
    if args.jobs < 1 or args.timeout <= 0:
        parser.error("--jobs and --timeout must be positive")

    try:
        files = set()
        for path in args.paths:
            if path.is_dir():
                files.update(p.resolve() for p in path.rglob("*.p4") if p.is_file())
            elif path.is_file():
                files.add(path.resolve())
            else:
                raise RuntimeError(f"{path}: no such file or directory")
        if not files:
            raise RuntimeError("no P4 files found")

        # Collect all proposals before writing so a formatter failure cannot
        # leave a partly formatted corpus. Sorting also makes diffs deterministic.
        changes = []
        failures = []
        with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as executor:
            pending = [
                (
                    path,
                    executor.submit(
                        format_file,
                        args.formatter.resolve(),
                        path,
                        args.timeout,
                        args.fail_on_fallback,
                    ),
                )
                for path in sorted(files)
            ]
            for path, future in pending:
                try:
                    proposal = future.result()
                    if proposal is not None:
                        changes.append(proposal)
                except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
                    failures.append(f"{path}: {error}")
        for failure in failures:
            print(f"p4fmt: {failure}", file=sys.stderr)
        if failures:
            print(
                f"{len(failures)} of {len(files)} P4 files could not be formatted.", file=sys.stderr
            )
            if args.write:
                return 2

        # Check mode still shows diffs for supported files when others fail.
        # Write mode only reaches here if every input was formatted successfully.
        for path, original, formatted in changes:
            if args.write:
                replace_file(path, original, formatted)
            else:
                print_diff(path, original, formatted)
        if args.write:
            print(f"Formatted {len(changes)} of {len(files)} P4 files.")
        elif changes:
            print(
                f"{len(changes)} of {len(files)} P4 files need formatting. "
                "Run cmake --build build --target p4fmt-fix-errors.",
                file=sys.stderr,
            )
        if failures:
            return 2
        return 1 if changes and not args.write else 0
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
        print(f"p4fmt: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
