# Repository Guidelines — graphs

## Purpose
This folder contains the graphs backend implementation and supporting code.

## Key Entry Points
- `backends/graphs/`: backend sources and local helpers.
- `backends/graphs/CMakeLists.txt`: build targets and options.
- `backends/graphs/README.md`: backend-specific usage notes.

## Tests & Outputs
- Test targets are backend-specific; see `backends/graphs/CMakeLists.txt` and any `*.cmake` files for custom targets.
- Idiomatic filtered runs: `ctest --test-dir build -j<N> --output-on-failure -R graphs`.
- Integration inputs and expected outputs live under `testdata/`.

## Navigation Tips
- Start from the backend driver/entrypoint in `backends/graphs/` and follow includes from there.
