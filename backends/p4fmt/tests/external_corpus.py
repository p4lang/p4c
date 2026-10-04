#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
# SPDX-License-Identifier: Apache-2.0

"""Check pinned DASH/PINS sources, and prove every file reaches the formatter.

Only this explicit target downloads sources. Cached bytes are verified against
SHA-256 hashes; revisions and complete file lists are reviewable in the manifest.
The source repositories and cache are never rewritten with formatted output.
"""

import argparse
import concurrent.futures
import hashlib
import json
import pathlib
import subprocess
import tempfile
import urllib.request

from corpus import check_file

PROBE = b"\n\nconst    bit<8>   p4fmt_probe   =   1 ;\n"
EXPECTED = b"const bit<8> p4fmt_probe = 1;\n"


def check(formatter, cache, repository, revision, path, digest):
    source = cache / repository.split("/")[-1] / path
    if not source.exists():
        url = f"https://raw.githubusercontent.com/{repository}/{revision}/{path}"
        with urllib.request.urlopen(url, timeout=60) as response:
            contents = response.read()
        if hashlib.sha256(contents).hexdigest() != digest:
            raise ValueError(f"download hash mismatch: {url}")
        source.parent.mkdir(parents=True, exist_ok=True)
        source.write_bytes(contents)
    original = source.read_bytes()
    if hashlib.sha256(original).hexdigest() != digest:
        raise ValueError(f"cache hash mismatch: {source}")
    status, detail = check_file(formatter, source, 20)
    if status == "failed":
        raise ValueError(f"{source}: {detail}")
    with tempfile.TemporaryDirectory(prefix="p4fmt-probe-") as directory:
        probe = pathlib.Path(directory) / source.name
        probe.write_bytes(original + PROBE)
        result = subprocess.run(
            [formatter, "--fail-on-fallback", probe], capture_output=True, timeout=20, check=True
        )
        if result.stderr or not result.stdout.endswith(EXPECTED):
            raise ValueError(f"{source}: did not format appended declaration: {result.stderr!r}")
        probe.write_bytes(result.stdout)
        probe_status, detail = check_file(formatter, probe, 20)
        if probe_status != "unchanged":
            raise ValueError(f"{source}: unstable probe: {detail}")
    return status


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--formatter", required=True, type=pathlib.Path)
    parser.add_argument("--cache", required=True, type=pathlib.Path)
    parser.add_argument("--jobs", default=4, type=int)
    args = parser.parse_args()
    manifest = json.loads(pathlib.Path(__file__).with_name("external-corpora.json").read_text())
    failed = False
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as executor:
        for corpus in manifest:
            pending = {
                executor.submit(
                    check,
                    args.formatter.resolve(),
                    args.cache,
                    corpus["repository"],
                    corpus["revision"],
                    path,
                    digest,
                ): path
                for path, digest in corpus["files"].items()
            }
            changed = 0
            failures = 0
            for future in concurrent.futures.as_completed(pending):
                try:
                    changed += future.result() == "changed"
                except Exception as error:
                    print(f"FAIL {pending[future]}: {error}", flush=True)
                    failures += 1
            print(
                f"{corpus['repository']}@{corpus['revision']}: "
                f"{len(pending) - failures}/{len(pending)} strict, stable, content-preserving "
                f"formats and active probes; {changed} originals reformatted",
                flush=True,
            )
            failed |= failures != 0
    return failed


if __name__ == "__main__":
    raise SystemExit(main())
