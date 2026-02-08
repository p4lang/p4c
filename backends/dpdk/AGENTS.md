# Repository Guidelines — dpdk

## Purpose
This folder contains the dpdk backend implementation and supporting code.

## Key Entry Points
- `backends/dpdk/`: backend sources and local helpers.
- `backends/dpdk/CMakeLists.txt`: build targets and options.
- `backends/dpdk/README.md`: backend-specific usage notes.

## Tests & Outputs
- Test targets are backend-specific; see `backends/dpdk/CMakeLists.txt` and any `*.cmake` files for custom targets.
- Idiomatic filtered runs: `ctest --test-dir build -j<N> --output-on-failure -R dpdk`.
- Integration inputs and expected outputs live under `testdata/`.

## Navigation Tips
- Start from the backend driver/entrypoint in `backends/dpdk/` and follow includes from there.
