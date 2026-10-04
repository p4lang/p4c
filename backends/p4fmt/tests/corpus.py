#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
# SPDX-License-Identifier: Apache-2.0

"""Check every P4 source, including negative tests and generated references.

Every invocation uses strict mode. The report separates changed and unchanged
results; the external acceptance test also probes active formatting in every
DASH/PINS source, including sources already in the chosen style.
"""

import argparse
import concurrent.futures
import json
import pathlib
import re
import subprocess
import tempfile
from collections import Counter, defaultdict

# Independently check that only whitespace outside comments and strings changed.
# Token boundary checks are covered separately by the C++ source tests.
CONTENT = re.compile(
    rb'//[^\r\n]*|/\*[\s\S]*?(?:\*/|\Z)|"(?:\\[\s\S]|[^"\\])*(?:"|\Z)' rb'|[^\t\n\v\f\r /"]+|[/"]'
)


def content(source):
    # C phase 2 runs before comment recognition. Map regex matches back to the
    # physical bytes so comments/strings retain even their embedded splices.
    if b"\\\n" not in source and b"\\\r\n" not in source:
        return b"".join(match.group() for match in CONTENT.finditer(source)), []
    offsets = []
    logical = bytearray()
    position = 0
    while position < len(source):
        splice = re.match(rb"\\\r?\n", source[position : position + 3])
        if splice:
            position += len(splice.group())
        else:
            offsets.append(position)
            logical.append(source[position])
            position += 1
    significant = b"".join(
        source[offsets[match.start()] : offsets[match.end() - 1] + 1]
        for match in CONTENT.finditer(logical)
    )
    return significant, re.findall(rb"\\\r?\n", source)


def check_file(formatter, source, timeout):
    original = source.read_bytes()
    try:
        first = subprocess.run(
            [formatter, "--fail-on-fallback", source], capture_output=True, timeout=timeout
        )
        if first.returncode or first.stderr:
            return "failed", f"exit {first.returncode}: {first.stderr.decode(errors='replace')}"
        if content(first.stdout) != content(original):
            return "failed", "changed non-whitespace content, a string, or a comment"
        with tempfile.TemporaryDirectory(prefix="p4fmt-corpus-") as directory:
            formatted = pathlib.Path(directory) / source.name
            formatted.write_bytes(first.stdout)
            second = subprocess.run(
                [formatter, "--fail-on-fallback", formatted], capture_output=True, timeout=timeout
            )
        if second.returncode or second.stderr:
            return "failed", f"second pass: exit {second.returncode}: {second.stderr!r}"
        if second.stdout != first.stdout:
            return "failed", "formatting is not idempotent"
        return ("changed" if first.stdout != original else "unchanged"), ""
    except subprocess.TimeoutExpired:
        return "failed", f"exceeded {timeout}s per invocation"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("formatter", type=pathlib.Path)
    parser.add_argument("testdata", type=pathlib.Path)
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--timeout", type=float, default=20)
    parser.add_argument("--report", type=pathlib.Path)
    args = parser.parse_args()
    files = sorted(path for path in args.testdata.rglob("*.p4") if path.is_file())
    if not files:
        parser.error("no P4 files found")
    counts = defaultdict(Counter)
    failures = {}
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as executor:
        pending = {
            executor.submit(check_file, args.formatter.resolve(), path, args.timeout): path
            for path in files
        }
        for index, future in enumerate(concurrent.futures.as_completed(pending), 1):
            path = pending[future].relative_to(args.testdata)
            status, detail = future.result()
            counts[path.parts[0]][status] += 1
            if status == "failed":
                failures[str(path)] = detail
                print(f"FAIL {path}: {detail[:2000]}", flush=True)
            if index % 1000 == 0:
                print(f"Checked {index}/{len(files)} files", flush=True)
    total = sum(counts.values(), Counter())
    report = {"total": dict(total), "suites": dict(counts), "failures": failures}
    print(json.dumps(report, indent=2, sort_keys=True), flush=True)
    if args.report:
        args.report.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    return bool(failures)


if __name__ == "__main__":
    raise SystemExit(main())
