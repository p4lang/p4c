# Repository Guidelines — ubpf

## Purpose
This folder contains the ubpf backend implementation and supporting code.

## Key Entry Points
- `backends/ubpf/`: backend sources and local helpers.
- `backends/ubpf/CMakeLists.txt`: build targets and options.
- `backends/ubpf/README.md`: backend-specific usage notes.

## Tests & Outputs
- Test targets are backend-specific; see `backends/ubpf/CMakeLists.txt` and any `*.cmake` files for custom targets.
- Idiomatic filtered runs: `ctest --test-dir build -j<N> --output-on-failure -R ubpf`.
- Integration inputs and expected outputs live under `testdata/`.

## Navigation Tips
- Start from the backend driver/entrypoint in `backends/ubpf/` and follow includes from there.
