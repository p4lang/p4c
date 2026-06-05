<!--
SPDX-FileCopyrightText: 2026 The P4 Language Consortium

SPDX-License-Identifier: Apache-2.0
-->

# Repository Guidelines — p4fmt

## Purpose
This folder contains the p4fmt backend implementation and supporting code.

## Key Entry Points
- `backends/p4fmt/`: backend sources and local helpers.
- `backends/p4fmt/CMakeLists.txt`: build targets and options.
- `backends/p4fmt/README.md`: backend-specific usage notes.

## Tests & Outputs
- Test targets are backend-specific; see `backends/p4fmt/CMakeLists.txt` and any `*.cmake` files for custom targets.
- Idiomatic filtered runs: `ctest --test-dir build -j<N> --output-on-failure -R p4fmt`.
- Integration inputs and expected outputs live under `testdata/`.

## Navigation Tips
- Start from the backend driver/entrypoint in `backends/p4fmt/` and follow includes from there.
