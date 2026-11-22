<!--
SPDX-FileCopyrightText: 2026 The P4 Language Consortium

SPDX-License-Identifier: Apache-2.0
-->

# Repository Guidelines — p4test

## Purpose
`p4test` is a backend-agnostic testing tool that exercises the P4 compiler front-end and mid-end on programs under `testdata/`.

## Key Entry Points
- `backends/p4test/README.md`: usage and intent.
- `backends/p4test/CMakeLists.txt`: compiler build targets.
- `testdata/P4Test.cmake`: test variants and runner arguments.
- `backends/p4test/`: implementation and supporting passes.

## Tests & Variants
- `testdata/CMakeLists.txt` selects suites based on the compiler targets in the build.
- Idiomatic filtered runs: `ctest --test-dir build -j<N> --output-on-failure -L '^(p4|err|p14_to_16)$'`.
- Integration inputs and expected outputs live under `testdata/`.

## Navigation Tips
- Start in `testdata/P4Test.cmake` to see how the suites invoke `backends/p4test/run-p4-sample.py`.
