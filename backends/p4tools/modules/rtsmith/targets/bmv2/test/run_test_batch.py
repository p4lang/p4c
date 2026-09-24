#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
# SPDX-License-Identifier: Apache-2.0

"""Generate RtSmith requests and verify them against BMv2's P4Runtime server."""

import argparse
import subprocess
import sys
import tempfile
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("p4_file", type=Path)
    parser.add_argument("--p4rtsmith", required=True, type=Path)
    parser.add_argument("--build-dir", required=True, type=Path)
    parser.add_argument("--seed", type=int, default=1)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[7]
    build = args.build_dir.resolve()
    with tempfile.TemporaryDirectory(prefix="rtsmith-", dir=build) as temporary:
        output = Path(temporary)
        subprocess.run(
            [
                str(args.p4rtsmith.resolve()),
                "--target",
                "bmv2",
                "--arch",
                "v1model",
                "--seed",
                str(args.seed),
                "--output-dir",
                str(output),
                str(args.p4_file.resolve()),
            ],
            check=True,
            cwd=build,
        )
        subprocess.run(
            [
                sys.executable,
                str(root / "backends/bmv2/run-bmv2-ptf-test.py"),
                str(root),
                str(args.p4_file.resolve()),
                "--use-nanomsg",
                "--nocleanup",
                "--testfile",
                str(Path(__file__).with_name("ptf_protobuf_base.py")),
                "--testdir",
                str(output),
            ],
            check=True,
            cwd=build,
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())
