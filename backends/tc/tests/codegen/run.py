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
import random
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


def build(compiler, directory, mode, flags, prepare=None, extra_sources=()):
    directory.mkdir(parents=True)
    fixtures = {"counter": "pna-codegen-counter.p4",
                "reject-error": "pna-codegen-reject-error.p4",
                "narrow-cache": "pna-codegen-narrow-cache.p4",
                "vlan": "pna-codegen-vlan.p4",
                "comparison": "pna-codegen-comparison.p4",
                "advance": "pna-codegen-advance.p4",
                "bitfields": "pna-codegen-bitfields.p4",
                "arithmetic": "pna-codegen-arithmetic.p4",
                 "register-init": "pna-register-init.p4",
                "deparser-bits": "tc-codegen-deparser-bits.p4"}
    source = ROOT / "testdata/p4_16_samples" / fixtures.get(mode, "pna-codegen-emission.p4")
    shutil.copyfile(source, directory / "case.p4")
    execute([compiler, "-I", ROOT / "p4include", *flags,
              directory / "case.p4", "-o", directory], directory, "p4")
    if prepare is not None:
        prepare(directory)
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
long host_vlan_push(struct __sk_buff *, unsigned short, unsigned short);
long host_vlan_pop(struct __sk_buff *);
#define bpf_skb_vlan_push(skb, proto, tci) host_vlan_push(skb, proto, tci)
#define bpf_skb_vlan_pop(skb) host_vlan_pop(skb)
#define bpf_redirect(port, flags) TC_ACT_REDIRECT
"""
    (directory / "ebpf_kernel.h").write_text(wrapper)
    for unit in ["parser", "control_blocks"]:
        execute(["clang-19", "-O2", "-D__KERNEL__", *includes,
                 "-Dp4tc_filter_fields=" + unit + "_fields", "-D_license=" + unit + "_license",
                 "-c", directory / f"case_{unit}.c", "-o", directory / f"{unit}.host.o"],
                directory, "host-" + unit)
    execute(["clang-19", "-O2", "-D__KERNEL__", *includes, HERE / "driver.c",
             *extra_sources, directory / "parser.host.o", directory / "control_blocks.host.o", "-o",
             directory / "driver"], directory, "link")
    return directory / "driver"


def cases(kind, variant="default"):
    """Rows are (frame, expected_wire_or_None, indices); reject specs, when
    given, are per-row exact (parser_return, parser_error, consumed_bits)."""
    ethernet = bytes.fromhex("00112233445566778899aabb")
    rows = []
    if kind == "arithmetic":
        width = int(variant.split("-")[0])
        parser_stage = variant.endswith("-parser")
        specs = []
        mask = (1 << width) - 1
        pairs = [(0, 0), (0, 1), (1, 0), (2, 3), (3, 3), (13, 14),
                 (14, 14), (20, 0), (21, 1), (1400, 1400), (1392, 1372),
                 (mask, 1), (mask, mask), (mask - 1, mask),
                 (1 << (width - 1), 2)]
        rng = random.Random(0x5044 + width)
        pairs += [(rng.getrandbits(width), rng.getrandbits(width)) for _ in range(16)]
        for op in range(9):
            for raw_x, raw_y in pairs:
                x, y = raw_x & mask, raw_y & mask
                expected = [int(x > ((y - 14) & mask)), int(((x - 20) & mask) != y),
                            (x - 3) & mask, (x - y) & mask, (x + 3) & mask,
                            (x + y) & mask, (x * y) & mask,
                            (((x - 3) & mask) * y + 1) & mask, (x + mask) & mask][op]
                frame = (ethernet + b"\x88\xb5" + bytes([op])
                         + struct.pack("!QQQ", x, y, 0))
                wire = frame[:-8] + expected.to_bytes(8, "big")
                error = 0
                if parser_stage and op < 2:
                    wire = frame if expected else None
                    error = 0 if expected else 2
                rows.append((frame, wire, []))
                specs.append((3, error, len(frame) * 8))
        return rows, specs
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
    if kind == "deparser-bits":
        # The deparser stores an unsigned and a signed four-bit field into one
        # byte. -1 in the signed field sets all eight bits, so only a masked
        # partial write keeps the neighbouring field.
        frame = ethernet + b"\x88\xb5\x00"
        return [(frame, frame[:-1] + b"\xaf", [])], None
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
    if kind == "narrow-cache":
        specs = []
        for flags, offset, expected in ((0, 0, True), (1, 1, True),
                                        (1, 5, False), (1, 0, False), (7, 255, False)):
            frame = ethernet + b"\x88\xb5" + ((flags << 13) | offset).to_bytes(2, "big") + b"\x5a"
            rows.append((frame, frame if expected else None, []))
            specs.append((3, 0, 136) if expected else (3, 6, 128))
        return rows, specs
    if kind == "reject-error":
        specs = []
        for flags in (0, 1, 7):
            for offset in (0, 1, 255) if flags == 7 else (0,):
                frame = ethernet + b"\x88\xb5" + ((flags << 13) | offset).to_bytes(2, "big") + b"\x5a"
                if offset == 0:
                    rows.append((frame, frame, []))
                    specs.append((3, 0, 136))
                else:
                    rows.append((frame, None, []))
                    specs.append((3, 6, 128))
        rows.append((ethernet + b"\x88\xb5", None, []))
        specs.append((3, 1, 112))
        rows.append((ethernet + b"\x88\xb6\0\0\x5a", None, []))
        specs.append((2, 0, 0))
        return rows, specs
    if kind == "vlan":
        def frame_for(mode):
            return ethernet + b"\x08\x00" + bytes([mode]) + b"payload"

        def tagged(frame, proto, tci):
            return frame[:12] + proto.to_bytes(2, "big") + tci.to_bytes(2, "big") + frame[12:]

        for present, proto, tci in ((1, 0x8100, 0x00a5), (1, 0x88a8, 0x2064),
                                    (0, 0x0000, 0x0000)):
            for mode in range(6):
                frame = frame_for(mode)
                if present == 0 and mode == 3:
                    expected, out_tag = frame, (1, 0x0064, 0x8100)
                elif mode == 5:
                    expected, out_tag = frame, (0, 0, 0)
                elif present == 1 and mode in (1, 2, 4):
                    expected = tagged(frame, proto, tci)
                    out_tag = (1, tci, proto) if mode == 1 else (0, 0, 0)
                else:
                    expected, out_tag = frame, (present, tci, proto)
                rows.append((frame, expected, [], (present, tci, proto), out_tag))
            # A tag left in the frame behind the metadata tag, or as the
            # outermost tag. Popping removes it from the bytes; behind a
            # metadata tag it becomes the metadata tag.
            for inner in (0x8100, 0x88a8):
                frame = tagged(frame_for(5), inner, 0x0123)
                out_tag = (1, 0x0123, inner) if present else (0, 0, 0)
                rows.append((frame, frame_for(5), [], (present, tci, proto), out_tag))
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
    data = b""
    for row in rows:
        frame, tag = row[0], (row[3] if len(row) > 3 else (0, 0, 0))
        data += struct.pack("=4I", len(frame), *tag) + frame
    result = execute([driver], directory, "frames", data=data)
    cursor = 0
    for number, row in enumerate(rows):
        frame, expected, indices = row[0], row[1], row[2]
        expected_tag = row[4] if len(row) > 4 else None
        header = struct.unpack_from("=6I11I", result.stdout, cursor)
        cursor += 68
        wire = result.stdout[cursor:cursor + header[4]]
        cursor += header[4]
        if expected is None:
            assert header[1] == 2 and (specs or header[0] == 2 or header[2] != 0), (
                number, "expected rejection", header)
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
        if expected_tag is not None:
            assert tuple(header[6:9]) == expected_tag, (number, "vlan tag", header[6:9],
                                                        expected_tag)
        assert header[5] == len(indices), (number, "counter calls", header)
        assert list(header[9:9 + len(indices)]) == indices, (number, "counter keys", header)
    assert cursor == len(result.stdout), "trailing driver output"
    return len(rows)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", required=True, type=Path)
    parser.add_argument("--mode",
                        choices=["counter", "emission", "parser", "deparser", "deparser-bits",
                                 "reject-error", "narrow-cache", "vlan", "comparison", "advance",
                                 "arithmetic", "bitfields"],
                        required=True)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if platform.system() != "Linux" or platform.machine() != "x86_64":
        print("SKIP: host execution requires Linux x86-64")
        return 77
    output = args.output or Path(tempfile.mkdtemp(prefix="p4tc-codegen-"))
    output.mkdir(parents=True, exist_ok=True)
    variants = {"default": []}
    if args.mode == "arithmetic":
        variants = {f"{width}-{stage}": [f"-DARITH_WIDTH={width}", f"-DARITH_STAGE={number}"]
                    for width in (4, 8, 12, 16, 24, 32, 48, 64)
                    for number, stage in enumerate(("parser", "control", "deparser", "action"))}
    if args.mode in ("parser", "reject-error"):
        variants.update({
            "one-range": ["--tc-outline-parser-range=parse_flags:finish"],
            "two-ranges": ["--tc-outline-parser-range=start:parse_flags",
                           "--tc-outline-parser-range=parse_flags:finish"],
            "cached-local": ["--tc-outline-parser-range=start:parse_flags",
                             "--tc-outline-parser-range=parse_flags:finish",
                             "--tc-outline-parser-bound=left:8191",
                             "--tc-outline-parser-local=left:cached_left:8191",
                             "--tc-outline-parser-offset-max=1522"],
        })
        if args.mode == "parser":
            variants["cached-runtime-name"] = [
                "--tc-outline-parser-range=start:parse_flags",
                "--tc-outline-parser-range=parse_flags:finish",
                "--tc-outline-parser-local=left:getPrimitive32:8191"]
    if args.mode == "narrow-cache":
        variants = {"inline": [],
                    "cached-local": ["--tc-outline-parser-range=start:parse_flags",
                                     "--tc-outline-parser-range=parse_flags:finish",
                                     "--tc-outline-parser-bound=left:8191",
                                     "--tc-outline-parser-local=left:cached_left:8191",
                                     "--tc-outline-parser-offset-max=1522"]}
    if args.mode == "emission":
        variants["frag-offset-name"] = ["-DFRAGMENT_OFFSET=fragOffset"]
    if args.mode == "advance":
        variants["outlined"] = ["--tc-outline-parser-range=skip:finish",
                                "--tc-outline-parser-offset-max=1522"]
    if args.mode == "deparser":
        variants["outlined"] = ["--tc-outline-deparser", "--tc-outline-deparser-max=1522"]
    if args.mode == "deparser-bits":
        variants["outlined"] = ["--tc-outline-deparser", "--tc-outline-deparser-max=1522"]
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
    if args.mode == "arithmetic":
        oracle = {frame: wire for frame, wire, _ in cases("arithmetic", "32-control")[0]}
        stf = (ROOT / "testdata/p4_16_samples/pna-codegen-arithmetic.stf").read_text()
        packets = [line.split() for line in stf.splitlines() if line.startswith(("packet ", "expect "))]
        rows = []
        for packet, expect in zip(packets[::2], packets[1::2]):
            assert packet[:2] == ["packet", "0"] and expect[:2] == ["expect", "0"]
            frame, wire = bytes.fromhex(packet[2]), bytes.fromhex(expect[2])
            assert wire == oracle[frame[:39]] + frame[39:], "STF differs from modulo oracle"
            rows.append((frame, wire, []))
        assert len(rows) == 11 and len(packets) == 22
        directory = output / "stf"
        directory.mkdir()
        record["stf_frames"] = run_frames(output / "32-control/driver", directory, rows)
    if args.mode == "advance":
        code = (output / "outlined/case_parser.c").read_text()
        assert "ebpf_outline_offset > 1522 * 8" in code, "one region ignored the offset bound"
    if args.mode == "deparser-bits":
        code = (output / "outlined/case_control_blocks.c").read_text()
        assert (
            "__p4tc_deparser_emit_nibbles_t(skb, &pkt_hdrs->nibbles" in code
        ), "the outlined deparser did not name the header argument"
        assert "&hdr->nibbles" not in code, "the outlined deparser kept a hard-coded name"
    if args.mode == "parser":
        two_ranges = ["--tc-outline-parser-range=start:parse_flags",
                      "--tc-outline-parser-range=parse_flags:finish"]
        for name, flags, program, diagnostic in [
            ("bound-one-range", ["--tc-outline-parser-range=parse_flags:finish",
                                 "--tc-outline-parser-bound=left:8191"],
             "emission", "require a second --tc-outline-parser-range"),
            ("local-one-range", ["--tc-outline-parser-range=parse_flags:finish",
                                 "--tc-outline-parser-local=left:cached_left:8191"],
             "emission", "require a second --tc-outline-parser-range"),
            ("no-hdr-one-range", ["--tc-outline-parser-range=parse_flags:finish",
                                  "--tc-outline-parser-no-hdr2"],
             "emission", "require a second --tc-outline-parser-range"),
            ("third-range", [*two_ranges, "--tc-outline-parser-range=skip:finish"],
             "emission", "at most two ranges"),
            ("offset-alone", ["--tc-outline-parser-offset-max=1522"],
             "emission", "requires --tc-outline-parser-range"),
            ("bound-field", [*two_ranges, "--tc-outline-parser-bound=nosuch:5"],
             "emission", "not a member of the user metadata"),
            ("local-name", [*two_ranges, "--tc-outline-parser-local=left:1bad:5"],
             "emission", "must be an identifier stem"),
            ("parser-local", ["--tc-outline-parser-range=parse_tail:finish"],
             "outline-local", "crosses the boundary"),
            ("parser-instance", ["--tc-outline-parser-range=parse_checksum:parse_last"],
             "outline-checksum", "is an extern instance"),
        ]:
            directory = output / ("negative-" + name)
            directory.mkdir()
            result = execute([args.compiler.resolve(), "-I", ROOT / "p4include", *flags,
                              ROOT / f"testdata/p4_16_samples/pna-codegen-{program}.p4",
                              "-o", directory], directory, "p4", success=False)
            assert result.returncode != 0 and diagnostic.encode() in result.stderr, (
                name, result.returncode, result.stderr)
            record.setdefault("negative", []).append(name)
        for local in ("skb", "hdr", "meta", "pkt", "while", "__cache"):
            directory = output / ("stem-" + local)
            directory.mkdir()
            execute([args.compiler.resolve(), "-I", ROOT / "p4include", *two_ranges,
                     "--tc-outline-parser-local=left:" + local + ":5",
                     ROOT / "testdata/p4_16_samples/pna-codegen-emission.p4",
                     "-o", directory], directory, "p4")
            code = (directory / "pna-codegen-emission_parser.c").read_text()
            assert "p4tc_cache_" + local + " = meta->left;" in code, local
            record.setdefault("stems", []).append(local)
    if args.mode in ("parser", "reject-error"):
        for name, flag, diagnostic in [
            ("syntax", "--tc-outline-parser-range=:finish", "expects START:END"),
            ("unknown", "--tc-outline-parser-range=missing:finish", "unknown parser states"),
            ("accept", "--tc-outline-parser-range=parse_flags:start", "accept transition"),
        ]:
            directory = output / ("negative-" + name)
            directory.mkdir()
            result = execute([args.compiler.resolve(), "-I", ROOT / "p4include", flag,
                              ROOT / "testdata/p4_16_samples/pna-codegen-emission.p4",
                              "-o", directory], directory, "p4", success=False)
            assert result.returncode != 0 and diagnostic.encode() in result.stderr, result.stderr
            record.setdefault("negative", []).append(name)
    if args.mode == "vlan":
        directory = output / "negative-deparser"
        directory.mkdir()
        result = execute([args.compiler.resolve(), "-I", ROOT / "p4include",
                          ROOT / "testdata/p4_16_samples/pna-codegen-vlan-deparser.p4",
                          "-o", directory], directory, "p4", success=False)
        assert result.returncode != 0 and b"Unexpected call to extern function" in result.stderr, (
            result.returncode, result.stderr)
        record.setdefault("negative", []).append("deparser")
    if args.mode == "narrow-cache":
        for name, flag, diagnostic in [
            ("missing", "--tc-outline-parser-local=missing:cached_missing:8191",
             "not a member of the user metadata"),
            ("narrow", "--tc-outline-parser-local=narrow:cached_narrow:8191",
             "scalar integer"),
            ("bound-narrow", "--tc-outline-parser-bound=narrow:5", "scalar integer"),
        ]:
            directory = output / ("negative-" + name)
            directory.mkdir()
            result = execute([args.compiler.resolve(), "-I", ROOT / "p4include",
                              "--tc-outline-parser-range=start:parse_flags",
                              "--tc-outline-parser-range=parse_flags:finish",
                              "--tc-outline-parser-offset-max=1522", flag,
                              ROOT / "testdata/p4_16_samples/pna-codegen-narrow-cache.p4",
                              "-o", directory], directory, "p4", success=False)
            assert result.returncode != 0 and diagnostic.encode() in result.stderr, (
                name, result.returncode, result.stderr)
            record.setdefault("negative", []).append(name)
    if args.mode == "parser":
        bounds = {"invalid": [], "valid": []}
        malformed = ["-1", "+1", "4294967296", "1522junk", "", " 1522", "1522 ",
                     "0x10", "1.0", "999999999999999999999999"]
        bounds_cases = [
            ("--tc-outline-parser-offset-max", "{v}", "decimal BYTES",
             malformed[:9] + ["268435456", malformed[9]]),
            ("--tc-outline-parser-local", "left:cached_left:{v}", "decimal MAX",
             [value for value in malformed if value]),
            ("--tc-outline-parser-bound", "left:{v}", "decimal uint32 MAX",
             [value for value in malformed if value]),
        ]
        for name, template, diagnostic, values in bounds_cases:
            for number, value in enumerate(values):
                directory = output / f"invalid-{name.strip('-')}-{number}"
                directory.mkdir()
                result = execute([args.compiler.resolve(), "-I", ROOT / "p4include",
                                  "--tc-outline-parser-range=start:parse_flags",
                                  "--tc-outline-parser-range=parse_flags:finish",
                                  name + "=" + template.format(v=value),
                                  ROOT / "testdata/p4_16_samples/pna-codegen-emission.p4",
                                  "-o", directory], directory, "p4", success=False)
                assert result.returncode != 0 and diagnostic.encode() in result.stderr, (
                    name, value, result.returncode, result.stderr)
                bounds["invalid"].append([name, value])
        valid = [
            ("--tc-outline-parser-offset-max", "{v}", [],
             ["0", "1", "1522", "0001522", "268435455"], "ebpf_outline_offset > {v} * 8"),
            ("--tc-outline-parser-bound", "left:{v}",
             ["--tc-outline-parser-local=left:cached_left:8191"],
             ["0", "1", "8191", "0008191", "4294967295"], "if (p4tc_cache_cached_left > {v})"),
            ("--tc-outline-parser-local", "left:cached_left:{v}", [],
             ["0", "1", "8191", "0008191", "4294967295"], "p4tc_cache_cached_left = meta->left;"),
        ]
        for name, template, extra, values, expected in valid:
            for number, value in enumerate(values):
                directory = output / f"valid-{name.strip('-')}-{number}"
                directory.mkdir()
                execute([args.compiler.resolve(), "-I", ROOT / "p4include",
                         "--tc-outline-parser-range=start:parse_flags",
                         "--tc-outline-parser-range=parse_flags:finish", *extra,
                         name + "=" + template.format(v=value),
                         ROOT / "testdata/p4_16_samples/pna-codegen-emission.p4",
                         "-o", directory], directory, "p4")
                code = (directory / "pna-codegen-emission_parser.c").read_text()
                assert expected.replace("{v}", str(int(value))) in code, (name, value)
                bounds["valid"].append([name, value])
        record["parser_bounds"] = bounds
    if args.mode == "deparser":
        bounds = {"invalid": [], "valid": []}
        for number, value in enumerate([
            "-1", "+1", "4294967296", "1522junk", "", " 1522", "1522 ",
            "0x10", "1.0", "268435456", "536870911", "999999999999999999999999",
        ]):
            directory = output / f"invalid-bound-{number}"
            directory.mkdir()
            result = execute([args.compiler.resolve(), "-I", ROOT / "p4include",
                              "--tc-outline-deparser", "--tc-outline-deparser-max=" + value,
                              ROOT / "testdata/p4_16_samples/pna-codegen-emission.p4",
                              "-o", directory], directory, "p4", success=False)
            assert result.returncode != 0 and b"expects decimal BYTES" in result.stderr, (
                value, result.returncode, result.stderr)
            assert not list(directory.glob("*_control_blocks.c")), value
            bounds["invalid"].append(value)
        for number, value in enumerate(["0", "1", "1522", "0001522", "268435455"]):
            directory = output / f"valid-bound-{number}"
            directory.mkdir()
            execute([args.compiler.resolve(), "-I", ROOT / "p4include",
                     "--tc-outline-deparser", "--tc-outline-deparser-max=" + value,
                     ROOT / "testdata/p4_16_samples/pna-codegen-emission.p4",
                     "-o", directory], directory, "p4")
            code = (directory / "pna-codegen-emission_control_blocks.c").read_text()
            assert f"offset_bits > {int(value)} * 8" in code, value
            bounds["valid"].append(value)
        record["deparser_bounds"] = bounds
    (output / "result.json").write_text(json.dumps(record, indent=2) + "\n")
    print(json.dumps(record, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
