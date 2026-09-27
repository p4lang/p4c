#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

"""Measure Testgen peak RSS at increasing test counts (Linux, GNU time)."""

import argparse
import json
import os
import signal
import subprocess
import time
from pathlib import Path


def run(binary, program, count, seed, timeout, directory):
    directory.mkdir(parents=True)
    rss_file = directory / "rss-kib.txt"
    command = [
        str(binary),
        "--target",
        "bmv2",
        "--arch",
        "v1model",
        "--test-backend",
        "STF",
        "--seed",
        str(seed),
        "--max-tests",
        str(count),
        "--out-dir",
        str(directory),
        str(program),
    ]
    started = time.monotonic()
    timed_out = False
    with (directory / "run.log").open("w") as log:
        process = subprocess.Popen(
            ["/usr/bin/time", "-f", "%M", "-o", str(rss_file), *command],
            stdout=log,
            stderr=subprocess.STDOUT,
            start_new_session=True,
        )
        try:
            process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            timed_out = True
            os.killpg(process.pid, signal.SIGKILL)
            process.wait()
    rss_lines = rss_file.read_text().splitlines() if rss_file.exists() else []
    peak_rss = int(rss_lines[-1]) if rss_lines and rss_lines[-1].isdigit() else None
    generated = len(list(directory.glob("*.stf")))
    return {
        "binary": str(binary),
        "command": command,
        "requested_tests": count,
        "generated_tests": generated,
        "peak_rss_kib": peak_rss,
        "wall_seconds": time.monotonic() - started,
        "exit_code": process.returncode,
        "timed_out": timed_out,
        "completed": process.returncode == 0 and generated == count,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, action="append", required=True)
    parser.add_argument("--program", type=Path, required=True)
    parser.add_argument("--counts", type=int, nargs="+", default=[100, 1000, 10000])
    parser.add_argument(
        "--max-peak-rss-mib",
        type=float,
        help="fail if any run exceeds this peak resident memory limit",
    )
    parser.add_argument("--seed", type=int, default=1000)
    parser.add_argument("--timeout", type=float, default=600)
    parser.add_argument("--out-dir", type=Path, required=True)
    args = parser.parse_args()
    if any(count <= 0 for count in args.counts) or args.timeout <= 0:
        parser.error("counts and timeout must be positive")
    if args.max_peak_rss_mib is not None and args.max_peak_rss_mib <= 0:
        parser.error("max-peak-rss-mib must be positive")
    if args.out_dir.exists():
        parser.error("out-dir must be new, so previous test files cannot affect counts")
    binaries = [binary.resolve(strict=True) for binary in args.binary]
    program = args.program.resolve(strict=True)
    output = args.out_dir.resolve()
    output.mkdir(parents=True)
    results = []
    for index, binary in enumerate(binaries):
        for sample, count in enumerate(args.counts):
            result = run(
                binary,
                program,
                count,
                args.seed,
                args.timeout,
                output / f"binary-{index}-sample-{sample}-{count}",
            )
            if args.max_peak_rss_mib is not None:
                result["max_peak_rss_kib"] = args.max_peak_rss_mib * 1024
                result["within_memory_limit"] = (
                    result["peak_rss_kib"] is not None
                    and result["peak_rss_kib"] <= result["max_peak_rss_kib"]
                )
            results.append(result)
            (output / "results.json").write_text(json.dumps(results, indent=2) + "\n")
            print(json.dumps(result), flush=True)
    return (
        0
        if all(
            result["completed"] and result.get("within_memory_limit", True) for result in results
        )
        else 1
    )


if __name__ == "__main__":
    raise SystemExit(main())
