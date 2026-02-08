# Repository Guidelines — ebpf

## Purpose
This folder contains the ebpf backend implementation and supporting code.

## Key Entry Points
- `backends/ebpf/`: backend sources and local helpers.
- `backends/ebpf/CMakeLists.txt`: build targets and options.
- `backends/ebpf/README.md`: backend-specific usage notes.

## Tests & Outputs
- Test targets are backend-specific; see `backends/ebpf/CMakeLists.txt` and any `*.cmake` files for custom targets.
- Idiomatic filtered runs: `ctest --test-dir build -j<N> --output-on-failure -R ebpf`.
- Integration inputs and expected outputs live under `testdata/`.

## Navigation Tips
- Start from the backend driver/entrypoint in `backends/ebpf/` and follow includes from there.
