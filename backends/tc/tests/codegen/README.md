<!--
SPDX-FileCopyrightText: 2026 Mojatatu Networks

SPDX-License-Identifier: Apache-2.0
-->

# tc code-generation regression checks

These tests compile small PNA programs twice: to BPF objects with Clang 19,
and to native code with explicit helper models. The native runner checks exact
packet bytes, parser rejection, and indirect-counter keys. It does not load BPF,
check verifier complexity, or modify a network interface.

Run the registered tests with:

```
ctest --test-dir build -R '^p4tc-codegen/' --output-on-failure
```

The BPF compilation uses the backend's usual runtime dependencies. Supply their
include roots through `C_INCLUDE_PATH` when they are outside this checkout.
Native execution requires Linux x86-64 because skb data addresses are 32-bit;
other hosts return CTest's skip code 77.

For retained artifacts, select an unused output directory:

```
python3 backends/tc/tests/codegen/run.py \
  --compiler build/backends/tc/p4c-pna-p4tc \
  --mode counter --output /tmp/p4tc-counter-check
```

Modes are `counter`, `emission`, `parser`, `deparser`, `reject-error`,
`narrow-cache`, `vlan`, `comparison`, `advance` and `bitfields`. The bitfields fixture
sends frames through a header whose fields start and end inside bytes, including
fields that straddle a byte, and compares the output
exactly for a walking one in every bit. The comparison fixture checks
sub-byte and 24-bit signed fields against negative constants and a `macaddr`
field against constants. The advance fixture has no guarding `verify`: it
checks the exact error for unaligned, truncated and wrapping advance counts,
and that an outlined region consuming more than 65535 bits rejects. The counter
fixture
checks the first and last element, eight dynamic indices, and conditional bitwise
and shift indices, including a branch-not-taken case. The emission fixture
checks all eight flag values, seven fragment offsets, truncation and parser
rejection. Parser variants exercise one range, two adjacent ranges and a cached
metadata field whose name is a prefix of another field. Parser mode also
checks strict decimal parsing of the offset, local and bound options:
twenty-nine malformed, signed, prefixed or out-of-range inputs are rejected,
and fifteen valid inputs keep their exact values in the generated code. Its
negative cases also reject options that need a missing region, unknown bound
fields, invalid local names and a parser local shared across a region boundary.
The reject-error fixture
verifies after a successful extraction and asserts the exact parser error and
consumed-bit count on every row in each parser variant, so a rejection cannot
silently change its selected error or cursor. The narrow-cache fixture caches
an eight-bit metadata field that is decremented and compared to zero, so an
inline parser and a cached-field parser must agree on wrapping assignment
semantics; its negative cases reject an unknown local field and a field
the target does not represent as a scalar. Deparser variants compare
inline and per-header helper emission against the same byte oracle.
Deparser mode also checks strict decimal output-bound parsing: twelve malformed
or out-of-range inputs are rejected, and five valid inputs retain their exact
numeric bounds. Extreme-bound cases check compiler acceptance and generated C;
they do not claim packet execution or verifier acceptance at those sizes. The
vlan fixture has twenty-four rows: it reads the tag both through the parser
metadata and through the control accessors, pushes a metadata tag into the frame
bytes, pops a metadata tag and writes it into the frame, asks for a tag on a frame
that has none, and pops a frame that still carries an 802.1Q or 802.1ad tag,
either as the outermost tag or behind a metadata tag that the pop then replaces
with it. Each row asserts the resulting frame bytes and the tag the stack
is left holding, so the helper's byte movement and the deparser's resize stay in
step. A negative case rejects a tag operation called from the deparser.

Each invocation keeps its generated sources, BPF objects, host objects, driver
output and `result.json`. A failed invocation retains the successful earlier
stages and the failing command's diagnostics.
