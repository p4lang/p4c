#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Mojatatu Networks
#
# SPDX-License-Identifier: Apache-2.0
"""Compile tc codegen fixtures and execute their generated C with host helper models.

The host runner uses low-address mmap on Linux x86-64. It neither loads BPF nor
changes the host network. C_INCLUDE_PATH may supply the normal runtime dependency
headers. Generated programs are also compiled for bpfel/v3 with Clang 19.
"""
import argparse
import json
from pathlib import Path
import platform
import re
import shutil
import struct
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]


def execute(command, directory, tag, data=None, success=True):
    result = subprocess.run([str(x) for x in command], input=data, capture_output=True)
    (directory / (tag + ".stdout")).write_bytes(result.stdout)
    (directory / (tag + ".stderr")).write_bytes(result.stderr)
    if success and result.returncode:
        raise AssertionError(f"{tag}: {result.returncode}\n{result.stderr.decode(errors='replace')}")
    return result


def build(compiler, directory, mode, flags):
    directory.mkdir(parents=True)
    fixtures = {'counter': 'pna-codegen-counter.p4', 'comparison': 'pna-codegen-comparison.p4', 'advance': 'pna-codegen-advance.p4', 'bitfields': 'pna-codegen-bitfields.p4'}
    source = ROOT / "testdata/p4_16_samples" / fixtures.get(mode, "pna-codegen-emission.p4")
    shutil.copyfile(source, directory / "case.p4")
    execute([compiler, "-I", ROOT / "p4include", *flags,
             directory / "case.p4", "-o", directory], directory, "p4")
    includes = ["-I" + str(directory), "-I" + str(ROOT / "backends/tc/runtime/include"),
                "-I" + str(ROOT / "backends/ebpf/runtime")]
    if Path("/usr/include/x86_64-linux-gnu").exists():
        includes.append("-I/usr/include/x86_64-linux-gnu")
    for unit in ["parser", "control_blocks"]:
        execute(["clang-19", "-O2", "--target=bpfel", "-mcpu=v3", "-D__KERNEL__",
                 *includes, "-c", directory / f"case_{unit}.c", "-o",
                 directory / f"{unit}.bpf.o"], directory, "bpf-" + unit)
    runtime = ROOT / "backends/ebpf/runtime/ebpf_kernel.h"
    wrapper = f'#include "{runtime}"\n' + """
#undef __ksym
#define __ksym
void *host_map(void);
long host_adjust_room(struct __sk_buff *, int, unsigned, unsigned long);
#undef BPF_MAP_LOOKUP_ELEM
#define BPF_MAP_LOOKUP_ELEM(map, key) host_map()
#undef REGISTER_TABLE
#define REGISTER_TABLE(...)
#undef BPF_ANNOTATE_KV_PAIR
#define BPF_ANNOTATE_KV_PAIR(...)
#define bpf_skb_adjust_room(skb, delta, mode, flags) host_adjust_room(skb, delta, mode, flags)
#define bpf_redirect(port, flags) TC_ACT_REDIRECT
"""
    (directory / "ebpf_kernel.h").write_text(wrapper)
    for unit in ["parser", "control_blocks"]:
        execute(["clang-19", "-O2", "-D__KERNEL__", *includes,
                 "-Dp4tc_filter_fields=" + unit + "_fields", "-D_license=" + unit + "_license",
                 "-c", directory / f"case_{unit}.c", "-o", directory / f"{unit}.host.o"],
                directory, "host-" + unit)
    execute(["clang-19", "-O2", "-D__KERNEL__", *includes, HERE / "driver.c",
             directory / "parser.host.o", directory / "control_blocks.host.o", "-o",
             directory / "driver"], directory, "link")
    return directory / "driver"


def cases(kind, variant="default"):
    """Rows are (frame, expected_wire_or_None, indices); reject specs, when
    given, are per-row exact (parser_return, parser_error, consumed_bits)."""
    ethernet = bytes.fromhex("00112233445566778899aabb")
    rows = []
    if kind == "comparison":
        specs = []
        mac = 0x021122334455
        for op, n4, n12, n24, address, expected in (
                (0, -1, 0, 0, 0, True), (0, 1, 0, 0, 0, False),
                (1, 0, -1, 0, 0, True), (1, 0, 1, 0, 0, False),
                (2, 0, 0, -5, 0, True), (2, 0, 0, 5, 0, False),
                (3, -1, 0, 0, 0, True), (3, 1, 0, 0, 0, False),
                (4, 0, 1, 0, 0, True), (4, 0, -1, 0, 0, False),
                (5, 0, 0, 0, mac, True), (5, 0, 0, 0, mac + 1, False),
                (6, 0, 0, 0, mac, True), (6, 0, 0, 0, 0x030000000000, False),
                (7, 0, 0, -5, 0, True), (7, 0, 0, -6, 0, False)):
            packed = ((n12 & 0xfff) << 4) | (n4 & 0xf)
            frame = (ethernet + b"\x88\xb5" + bytes([op]) + packed.to_bytes(2, "big")
                     + (n24 & 0xffffff).to_bytes(3, "big")
                     + address.to_bytes(6, "big"))
            rows.append((frame, frame if expected else None, []))
            specs.append((3, 0, 208))
        return rows, specs
    if kind == "bitfields":
        length = 36
        prefix = ethernet + b"\x88\xb5"
        patterns = [bytes(length), b"\xff" * length, b"\x5a" * length, bytes(range(1, length + 1))]
        patterns += [(1 << bit).to_bytes(length, "big") for bit in range(length * 8)]
        return [(prefix + pattern, prefix + pattern, []) for pattern in patterns], None
    if kind == "advance":
        prefix = ethernet + b"\x88\xb5"

        def frame(bits, payload):
            return prefix + bits.to_bytes(4, "big") + payload
        rows = [(frame(0, b"\xaa\xbb\0"), None, []),
                (frame(8, b"\xaa\xbb\xcc\0"), None, []),
                (frame(7, b"\xaa\xbb"), None, []),
                (frame(0x10, b"\xaa"), None, []),
                (frame(0xfffffff8, b"\xaa\xbb"), None, []),
                (frame(65512, bytes(8300)), None, []),
                (frame(65528, bytes(8300)), None, [])]
        # A helper's cursor starts at zero, so the wrapping count cannot overflow
        # there and the packet bound rejects it instead.
        outlined = variant == "outlined"
        specs = [(3, 0, 160), (3, 0, 168), (3, 6, 144), (3, 1, 144),
                 (3, 1, 144) if outlined else (3, 6, 144), (3, 0, 65672),
                 (3, 6, 144) if outlined else (3, 0, 65688)]
        return rows, specs
    if kind == "counter":
        for index in range(8):
            frame = ethernet + index.to_bytes(2, "big") + b"payload"
            indices = [1, 8, index + 1]
            if index != 0:
                indices.extend([index + 1, (index >> 1) + 1])
            rows.append((frame, frame, indices))
        return rows, None
    for flags in range(8):
        for offset in [0, 1, 31, 255, 256, 4095, 8191]:
            frame = ethernet + b"\x88\xb5" + ((flags << 13) | offset).to_bytes(2, "big") + b"\x5a"
            rows.append((frame, frame, []))
    for length in [0, 1, 13, 14, 15, 16]:
        rows.append((rows[0][0][:length], None, []))
    rows.append((ethernet + b"\x88\xb6\0\0\x5a", None, []))
    return rows, None


def run_frames(driver, directory, rows, specs=None):
    data = b"".join(struct.pack("=I", len(frame)) + frame for frame, _, _ in rows)
    result = execute([driver], directory, "frames", data=data)
    cursor = 0
    for number, (frame, expected, indices) in enumerate(rows):
        header = struct.unpack_from("=6I8I", result.stdout, cursor)
        cursor += 56
        wire = result.stdout[cursor:cursor + header[4]]
        cursor += header[4]
        if expected is None:
            assert header[1] == 2 and (specs or header[0] == 2 or header[2] != 0), (number, "expected rejection", header)
            if specs:
                assert (header[0], header[2], header[3]) == specs[number], (
                    number, "rejection", specs[number], header)
        else:
            assert header[0] == 3, (number, "parser return", header)
            assert header[1] == 7 and header[2] == 0, (number, "expected forwarding", header)
            assert wire == expected, (number, "wire", wire.hex(), expected.hex())
            if specs:
                assert (header[0], header[2], header[3]) == specs[number], (
                    number, "parse", specs[number], header)
        assert header[5] == len(indices), (number, "counter calls", header)
        assert list(header[6:6 + len(indices)]) == indices, (number, "counter keys", header)
    assert cursor == len(result.stdout), "trailing driver output"
    return len(rows)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", required=True, type=Path)
    parser.add_argument("--mode",
                        choices=['counter', 'emission', 'comparison', 'advance', 'bitfields'],
                        required=True)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if platform.system() != "Linux" or platform.machine() != "x86_64":
        print("SKIP: host execution requires Linux x86-64")
        return 77
    output = args.output or Path(tempfile.mkdtemp(prefix="p4tc-codegen-"))
    output.mkdir(parents=True, exist_ok=True)
    variants = {"default": []}
    if args.mode == "emission":
        variants["frag-offset-name"] = ["-DFRAGMENT_OFFSET=fragOffset"]
    record = {"mode": args.mode, "variants": {}, "scope": "BPF compilation and modeled host execution"}
    for name, flags in variants.items():
        directory = output / name
        driver = build(args.compiler.resolve(), directory, args.mode, flags)
        rows, specs = cases(args.mode, name)
        count = run_frames(driver, directory, rows, specs)
        record["variants"][name] = {"frames": count, "flags": flags}
        if args.mode == "counter":
            record["variants"][name]["counter_calls"] = sum(len(row[2]) for row in rows)
        if specs:
            record["variants"][name]["rejection_specs"] = specs
    (output / "result.json").write_text(json.dumps(record, indent=2) + "\n")
    print(json.dumps(record, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
