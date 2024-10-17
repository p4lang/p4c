# CLOT allocator and metric {#clot_alloc_and_metric}

This documents the design of the CLOT allocator and the metric associated with
CLOT allocation.

## Definitions

When an object (field, slice, byte, etc.) is allocated to both PHVs and to a
CLOT, we say that the object is double-allocated.

We categorize the field slices in the program, as follows:
- _Checksum_: These are slices whose values are replaced with checksum
  calculations in the deparser. While checksum slices will not necessarily have a
  %PHV allocation, it is only useful to allocate these slices to CLOTs when doing
  so would allow us to join groups of slices into a larger CLOT.
- _Modified_: These are non-checksum slices satisfying any of the following
  conditions.
  - The slice is written by the MAU pipeline.
  - The slice is aliased with a modified slice via `@pa_alias.`
  - The slice is packed with a modified slice in the same header: the two
    slices have bits that occupy the same byte in the packet.
  These slices will be PHV-allocated and deparsed from their %PHV containers.
  It is only useful to double-allocate these slices when doing so would allow
  us to join groups of unmodified slices into a larger CLOT.
- _Read-only_: These are unmodified non-checksum slices that are read by the
  MAU pipeline. While these will be PHV-allocated, it is useful to
  double-allocate these slices, so that their %PHV containers do not have to be
  live for the entire MAU pipeline.
- _Unused_: These are unmodified non-checksum slices that are not read by the
  MAU pipeline. Since these do not have to be PHV-allocated, we get the most
  benefit from CLOT-allocating these slices.

A CLOT-allocated field slice is overwritten if the value in the slice’s CLOT is
replaced by a value from a %PHV container or by a calculated checksum during
deparsing. More precisely, a CLOT-allocated slice is overwritten if it
satisfies any of the following conditions.
- The slice is a checksum slice.
- The slice is modified.
- The slice is co-allocated to a %PHV container with a modified slice.
- The slice is co-allocated to a %PHV container with a slice that is not
  allocated to the same CLOT (i.e., the %PHV container straddles the CLOT
  boundary).

## Objective

The objective of the CLOT allocator is two-fold:
1. CLOT-allocate as many unused and read-only slices as possible, while
   minimizing the number of CLOTs used.
2. Identify checksum and modified slices and compute their offsets within their
   CLOT. This is so that we can produce the correct overwrite offsets for the
   slices’ %PHV containers and checksum values in the deparser. Because CLOT
   allocation occurs before %PHV allocation, the CLOT allocator needs to do
   this itself.

## CLOT layout constraints

The following are the layout constraints for CLOT allocation.
- Each CLOT can hold up to 64 bytes of contiguous packet data. The deparser
  emits this data in an all-or-none fashion.
- Each parser state can issue up to 2 CLOTs.
- Consecutive CLOTs in the input packet must be separated by an inter-CLOT gap
  of at least three bytes.
  - Exception. Consecutive CLOTs in the input packet may be directly adjacent
    (i.e., separated by zero bytes) if all of the following conditions are met.
    - Whenever the deparser emits the CLOT that came second in the input
      packet, it also emits the CLOT that came first.
    - Whenever the deparser emits both CLOTs, it emits them directly
      adjacently, in the same order as they are parsed.
- There are a total of 64 unique CLOT tags while only up to 16 CLOTs can be
  live in a packet. These limits apply on a per-gress basis.
- The offset + length in the input packet of each CLOT can be no larger than
  384 bytes.

## CLOT-eligible field slices

A field slice is CLOT-eligible if it is part of a CLOT-eligible field. A field
is CLOT-eligible if it is extracted by the parser and satisfies all of the
following properties.
- The field’s header is not added by the MAU pipeline. This ensures that a
  CLOT allocated to the field is not deparsed when the field is not part of
  the input packet, but is added by the MAU pipeline.
- The field is emitted by the deparser.
- The field is not used in multiple checksum-update computations. It is fine,
  however, to be part of a single checksum-update computation that is used to
  update multiple checksum fields.
- The field is extracted at most once by the parser: if the field is extracted
  in multiple parser states, then those states are mutually exclusive.
- The field has the same bit-in-byte offset (i.e., the same bit offset, modulo
  8) in all parser states that extract the field.
- The parser extracts the full width of the field from the packet.
- The field is extracted on all parser paths through that any state that marks
  the field’s header as valid.

## CLOT candidates

A CLOT candidate consists of a maximal sequence of CLOT-eligible field slices
whose extracts satisfy all of the following properties.
- A contiguous, byte-aligned sequence of bits is extracted from the packet.
- All extracted field slices are contiguous when emitted by the deparser.
- All extracted field slices have emits that are controlled by the same set of
  POV bits. This ensures that all field slices in a CLOT are deparsed together by
  the P4 program.
- Neither the first nor last field slice in the sequence of extracts is
  modified or a checksum.1
- The offset in the input packet of the last bit of the sequence of extracts is
  no larger than 384 × 8 = 3072.
- The extracts are all made in the same set of parser states: if a parser state
  extracts one field slice in a candidate, then also it extracts all other field
  slices in the same candidate.

While CLOTs can span more than one parser state, this allocation algorithm does
not use this property.

### Identifying CLOT candidates

An initial set of CLOT candidates is identified and created through the
following sequence of phases.
1. Finding pseudoheaders. With the decaf optimization, headers are no longer
   the units of deparsing: fields may be emitted separately from the rest of their
   header. Fields are therefore grouped into pseudoheaders, which are contiguous
   sequences of fields emitted by the deparser, with all fields sharing the same
   set of POV bits.2
2. Grouping field extracts by pseudoheader. CLOT-eligible fields are grouped by
   pseudoheader. Each CLOT-eligible field is associated with a map from each
   parser state in which it is extracted to the field’s offset from the start of
   the parser state.
3. Creating CLOT candidates. The sequence of fields in each pseudoheader is
   traversed, and the parser-state and field-offset information from Phase 2 is
   used to find corresponding contiguous sequences of field extracts that satisfy
   the requirements for CLOT candidacy.

## Allocation algorithm {#clot_alloc}

The CLOT allocator uses a greedy algorithm to allocate CLOTs in each gress
independently. For each gress, it identifies CLOT candidates and allocates them
in descending order of number of unused bits. Ties are broken by preferring
candidates with more read-only bits. As each candidate is allocated, the
remaining candidates are adjusted by splitting, shrinking, or removing from
candidacy as needed, to account for the inter-CLOT-gap requirement. This
adjustment must account for overlapping fields and look-ahead extracts. The
numbers of unused and read-only bits in each candidate are updated to account
for this adjustment. Shrinking of candidates is done at the granularity of
slices.

### Candidate adjustment

After allocating a CLOT candidate C, we adjust the remaining candidates by
splitting, shrinking, or removing from candidacy as needed to ensure the bytes
in the remaining candidates do not overlap with C in the input packet. We
further remove the beginning and end of every remaining unallocated candidate
C’ to account for the inter-CLOT gap. First, a couple of definitions.

- Define a CLOT-candidate gap function, GAPS, which summarizes all possible
  gaps between two CLOT candidates: GAPS(C, C’) gives the set of all possible
  gap sizes between C and C’ when C is parsed before C’ in the input packet.
  Some examples:
  - ∅ = GAPS(C, C’) if C is never parsed before C’.
  - 0 ∈ GAPS(C, C’) if there is a path through the parser in which C is parsed
    before C’ and the two are immediately adjacent.
  - 1 ∈ GAPS(C, C’) if there is a path through the parser in which C is parsed
    before C’ and the two are separated in the packet by 1 byte.
- Define a predicate, SEP, for determining when adjacent CLOT candidates can
  become separated during deparsing.
  - Assume 0 ∈ GAPS(C1, C3) and C1 is deparsed before C3.
  - SEP(C1, C3) = true exactly when any of the following is satisfied:
    - The deparser emits a constant between C1 and C3.
    - There is a pseudoheader H2 that is deparsed between C1 and C3,
      satisfying any of the following:
      - H2 might be added by the MAU pipeline.
      - There is a path through the parser in which C1 is parsed immediately
	before C3, and H2 is also parsed.

If C may be parsed before C’ (i.e., GAPS(C, C’) ≠ ∅), then we remove the first
few bytes at the beginning of C’ if any of the following is satisfied.
- C never appears immediately before C’ in the input packet: 0 ∉ GAPS(C, C’)).
- When C appears before C’ in the packet, the two might be separated by 1 or 2
  bytes: GAPS(C, C’) ∩ {1, 2} ≠ ∅.
- C might be set invalid by the MAU pipeline when C’ remains valid.
- C’ is deparsed before C.
- C and C’ can become separated during deparsing: SEP(C, C’) = true.

Dually, if C’ may be parsed before C (i.e., GAPS(C’, C) ≠ ∅), then we remove
the last few bytes at the end of C’ if any of the following is satisfied.
- C’ never appears immediately before C in the input packet: 0 ∉ GAPS(C’, C)).
- When C’ appears before C in the packet, the two might be separated by 1 or 2
  bytes: GAPS(C’, C) ∩ {1, 2} ≠ ∅.
- C’ might be set invalid by the MAU pipeline when C remains valid.
- C is deparsed before C’.
- C and C’ can become separated during deparsing: SEP(C’, C) = true.

These adjustments are not mutually exclusive; adjustments may be necessary at
both the beginning and end of C’. The number of bytes removed by these
adjustments is computed to be the smallest amount needed to achieve the
required inter-CLOT gap separation between C and C’ on all paths through the
parser.

## Allocation adjustment {#clot_alloc_adjust}

Once %PHV allocation is complete, the allocated CLOTs are adjusted to account
for %PHV container packing. Consider the example shown below. The CLOT begins
with the field f2, which is double-allocated, read-only, and packed into a %PHV
container with f1. The %PHV container also has a slice of f3 to ensure the
container is byte-aligned.

Because f1 is modified, the %PHV container will be deparsed, and will overwrite
the CLOT. But the overwriting container straddles the CLOT boundary, which is
not permitted. The CLOT, therefore, needs to be adjusted to exclude those
portions that overlap the %PHV container. A similar situation occurs for
overwrites straddling the end of CLOTs.

```text
header h_t {
  bit<8>  f1;  // modified
  bit<4>  f2;  // read-only
  bit<12> f3;  // unused
  ...


 PHV container           CLOT
       │                   │
       │                   │
       │                   ▼
       ▼     ──── ──── ──── ──── ──── ────
┌ ─ ─ ─ ─ ─ ┴ ─ ─ ─ ─ ─                   │
 ┌──────────┬────┬─────┴──────────┐       │
││    f1    │ f2 │       f3       │  ...  │
 └──────────┴────┴─────┬──────────┘       │
└ ─ ─ ─ ─ ─ ┬ ─ ─ ─ ─ ─
            └ ──── ──── ──── ──── ──── ───┘
```

To make these adjustments, we shrink all allocated CLOTs until they neither
start nor end with an overwritten field slice. If the entire CLOT is
overwritten, then the CLOT is unnecessary, and is de-allocated.

## Metrics

We form a metric for CLOT allocation by first identifying header bis that can
benefit from CLOT allocation. These are bits satisfying all of the following
conditions.

- Belongs to a field whose full width is extracted by the parser.
- Emitted by the deparser.
- Not contained in a header that is added by the MAU pipeline.
- Not written by the MAU pipeline (either directly, or through an alias).
- Not used in multiple checksum-update computations.

We then report:
- How many of these bits do not have a CLOT allocation. This measures “lost
  opportunity”, since these bits could be allocated to CLOTs (ignoring the
  inter-CLOT gap constraint).
- How many of these bits have both a CLOT allocation and a %PHV allocation.
  This measures redundancy in the %PHV allocation, since there is no benefit to
  having these bits be allocated to PHVs.
In keeping with the other metrics reported by the compiler, lower is better
for these metrics.

We could categorize these bytes into sub-metrics, as follows:

- Bytes that fall into the 3-byte inter-CLOT gap requirement (hardware
  limitation).
- Bytes that are not CLOT-allocated for other reasons.

Not all of these sub-metrics will necessarily be indicative of a better CLOT
allocation, however. For example, an allocator that allocates an additional
CLOT might reduce the overall number of bytes that are not CLOT-allocated, but
increase the number of inter-CLOT gap bytes.
