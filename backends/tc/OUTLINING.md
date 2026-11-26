<!--
SPDX-FileCopyrightText: 2026 Mojatatu Networks

SPDX-License-Identifier: Apache-2.0
-->

# Opt-in parser and deparser outlining

Inlining a large parser or deparser can exhaust the BPF verifier's path budget.
The tc backend can instead emit selected regions as non-inlined BPF functions.
This is target code generation: it does not add P4 externs or change PNA APIs.
All outlining options are disabled by default.

## A minimal example

A flat, byte-aligned header chain with a per-state branch is the smallest
program shape that shows why these options exist:

```p4
header tagval_t { bit<8> tag; bit<8> val; }
struct headers_t { tagval_t h0; tagval_t h1; /* ... */ tagval_t h7; }
struct metadata_t { bit<16> total; }

parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta,
              in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract(hdr.h0);
        meta.total = (bit<16>)hdr.h0.val;
        transition select(hdr.h0.tag) {
            0x01: s1;
            0x02: s2;
            default: reject;
        }
    }
    /* s1..s5: extract(hdr.hK), total += hK.val, select(hK.tag)
       {0x01: s(K+1); 0x02: s(K+2); default: reject} — two adjacent regions
       are chosen so every packet access stays inside a helper. */
    state s6 {
        pkt.extract(hdr.h6);
        meta.total = meta.total + (bit<16>)hdr.h6.val;
        transition select(hdr.h6.tag) {
            0x01: s7;
            0x02: s7;
            default: reject;
        }
    }
    state s7 {
        pkt.extract(hdr.h7);
        meta.total = meta.total + (bit<16>)hdr.h7.val;
        transition accept;
    }
}
control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta,
                 in pna_main_output_metadata_t ostd) {
    apply { pkt.emit(hdr.h0); pkt.emit(hdr.h1); /* ... */ pkt.emit(hdr.h7); }
}
```

The regions absorb every packet access, so the caller contains no packet
dereference after either helper call, and the helpers receive a bounded
entry offset:

```text
p4c-pna-p4tc example.p4 -o out \
  --tc-outline-parser-range=start:s6 \
  --tc-outline-parser-range=s6:accept \
  --tc-outline-parser-offset-max=1522 \
  --tc-outline-deparser \
  --tc-outline-deparser-max=1522
```

Measured on the `tests/codegen` toolchain (this example compiles to BPF and
loads as a standalone tc program; the verifier threshold of a program of this
shape is toolchain- and kernel-dependent):

| Generated unit | N=8 inline | N=8 outlined |
|---|---:|---:|
| parser processed instructions | 923 | 662 |
| control (deparser) processed instructions | 1197 | 313 |

At eight states the program also verifies when fully inlined; the same shape
is what a larger program grows into, and larger instances of it show the
helper isolation shrinking the caller's verification work. The motivating
case remains a program whose fully inlined parser or deparser exceeds the
verifier's budget. The static instruction count is not the relevant number;
use the verifier's processed-instruction result.

## Parser regions

`--tc-outline-parser-range=START:END` outlines the lowered parser graph reachable
from START before END. END remains in the caller. Internal branches and rejection
are supported; entering the region anywhere except START, or accepting before
END, is rejected. State names refer to the lowered parser, so a state eliminated
by simplification cannot be selected. A second adjacent region can be selected by repeating the option
(`--tc-outline-parser-range=START:END` twice).

The helper takes skb, header and metadata pointers and an entry bit offset. Its
result carries rejection, parser error and a 16-bit consumed-bit count. Regions
must therefore consume at most 65535 bits; a helper that consumes more rejects
with `ParserInvalidArgument`. The qualified use is byte-aligned, fixed-width
extraction. A parser-local variable used both inside a region and outside it is
rejected with a diagnostic, because each helper has a private copy. This is an
opt-in mechanism, not an automatic partitioner.

Two boundary conditions are load-bearing for the helper to verify:

- The caller must not dereference the packet after a helper call. The returned
  offset is unknown to the verifier, so an extract in a caller state after
  `END` is rejected. Shape the regions so the last helper absorbs the final
  packet access (for example, chain the last region's `END` to `accept`), and
  keep only metadata work in caller states after a helper call.
- A helper's own verification requires a bounded entry offset. The
  `--tc-outline-parser-offset-max` check is what makes the helper's bounds
  proof work; without it the helper is rejected on its first packet access.

Additional options support bounded metadata-driven regions:

| Option | Meaning |
|---|---|
| `--tc-outline-parser-bound=FIELD:MAX` | Check a metadata field at entry to the second region. MAX is a decimal uint32. |
| `--tc-outline-parser-local=FIELD:LOCAL:MAX` | Cache that metadata field in a scalar local of the field's own C type in the second region and write it back on return, so assignments keep the field's truncation and wrapping semantics. Only boolean and scalar integer fields can be cached; fields the target represents as a byte array are rejected with a diagnostic. MAX is a decimal uint32 and is retained in the option record; use the explicit bound option to enforce it. |
| `--tc-outline-parser-offset-max=BYTES` | Reject unaligned or larger entry offsets for both regions. BYTES is decimal 0–268435455, so the bit bound stays inside the helper's signed 32-bit return type; 0 permits only an entry offset of zero. It is required for the helpers' own verification. |
| `--tc-outline-parser-no-hdr2` | Omit the second helper's header pointer; select this only for a region that does not access headers. |

Every numeric bound is parsed strictly: decimal digits only, the whole argument
consumed, no sign or whitespace, and a value inside the stated range; malformed,
prefixed or overflowing input is an error.

The field argument must be a metadata member and the local argument a
spelling-only identifier stem. The compiler prefixes the stem and passes it
through the reference map, so it cannot shadow parser temporaries, runtime
declarations or C keywords.
`--tc-outline-parser-bound`, `--tc-outline-parser-local` and
`--tc-outline-parser-no-hdr2` configure the second region, so they are errors
without a second `--tc-outline-parser-range`. That option and
`--tc-outline-parser-offset-max` are errors without
`--tc-outline-parser-range`.
A cached field keeps its declared type in the helper, so a `bit<8>` field still
wraps at eight bits inside the outlined region; the local and bound options report an
error for fields whose target representation is not a scalar (for example a
width that becomes a byte array, or a struct).
The caller is responsible for choosing consistent bounds and non-overlapping
regions. Generated helpers still need BPF compilation, verifier loading and
packet tests for the selected program and target kernel.

## Deparser headers

`--tc-outline-deparser` moves fixed-layout top-level header emission into one
helper per header type. The deparser still computes output size and adjusts skb
geometry before calling the helpers. `--tc-outline-deparser-max=BYTES` bounds
helper entry offsets and the resulting output size; its default is 1522 bytes.
This is an explicit limit for this option, not a universal tc frame limit.
BYTES must contain only decimal digits and lie in 0–268435455, so the bit
bound fits the helper's signed 32-bit return value. Signs, whitespace, suffixes
and overflow are rejected. Leading zeros are accepted as decimal. Zero is a
valid bound that permits no nonempty emitted header.

The qualified shape is a flat header struct with ordinary fixed-width emits.
Header stacks, variable-width emits and arbitrary deparser expressions need
separate qualification. Per-header functions intentionally stop LLVM from
inlining all writes into one verification unit.

## Regression scope

`tests/codegen/` checks BPF compilation and modeled native packet execution.
Those checks do not prove verifier acceptance or kernel-helper behavior.
Keep target loading and interface-packet evidence separate from these tests.
