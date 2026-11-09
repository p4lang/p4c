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

Modes: counter, comparison, advance.

The comparison fixture checks narrow signed values and network-order MAC
fields against constants. The advance fixture exercises counts without a
guarding verify; rejection checks include the error and consumed cursor.

Each invocation keeps its generated sources, BPF objects, host objects, driver
output and `result.json`. A failed invocation retains the successful earlier
stages and the failing command's diagnostics.
