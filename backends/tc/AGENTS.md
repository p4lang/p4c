# Repository Guidelines — tc

## Purpose
This folder contains the tc backend implementation and supporting code.

## Key Entry Points
- `backends/tc/`: backend sources and local helpers.
- `backends/tc/CMakeLists.txt`: build targets and options.
- `backends/tc/README.md`: backend-specific usage notes.

## Tests & Outputs
- Test targets are backend-specific; see `backends/tc/CMakeLists.txt` and any `*.cmake` files for custom targets.
- Idiomatic filtered runs: `ctest --test-dir build -j<N> --output-on-failure -R tc`.
- Integration inputs and expected outputs live under `testdata/`.

## Navigation Tips
- Start from the backend driver/entrypoint in `backends/tc/` and follow includes from there.
