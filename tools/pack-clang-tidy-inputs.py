#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
# SPDX-License-Identifier: Apache-2.0

"""Share generated sources and dependency headers between analysis CI shards.

The shards use the same checkout/build paths and system packages as preparation.
Compilation/link outputs are intentionally unnecessary for clang-tidy.
"""

import argparse
import tarfile
from pathlib import Path

INPUT_SUFFIXES = {
    ".h",
    ".hh",
    ".hpp",
    ".hxx",
    ".inc",
    ".ipp",
    ".tcc",
    ".def",
    ".c",
    ".cc",
    ".cpp",
    ".cxx",
}


def pack(build, output, extra=()):
    count = 0
    with tarfile.open(output, "w:gz", dereference=True) as archive:
        for path in sorted(build.rglob("*")):
            if path.is_file() and (
                path.suffix.lower() in INPUT_SUFFIXES or path.name == "compile_commands.json"
            ):
                archive.add(path, arcname=Path(build.name) / path.relative_to(build))
                count += 1
        for directory in extra:
            archive.add(directory, arcname=directory)
    return count


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument(
        "--extra",
        type=Path,
        action="append",
        default=[],
        help="Also bundle a checkout-relative dependency directory",
    )
    args = parser.parse_args()
    if not (args.build / "compile_commands.json").is_file():
        parser.error("The build must contain compile_commands.json")
    for directory in args.extra:
        if directory.is_absolute() or ".." in directory.parts or not directory.is_dir():
            parser.error("--extra must be an existing checkout-relative directory")
    count = pack(args.build, args.output, args.extra)
    print(f"Packed {count} analysis inputs ({args.output.stat().st_size / 1024**2:.1f} MiB).")
