# Repository Guidelines — tofino

## Purpose
The Tofino backend targets the Tofino 1/2 chip family. It transforms frontend P4 IR into a Tofino-specific pipeline IR and performs resource allocation for the target architecture.

## Key Entry Points
- `backends/tofino/bf-p4c/README.md`: design overview, dependencies, and terminology.
- `backends/tofino/CMakeLists.txt`: build targets and options.
- `backends/tofino/bf-p4c/`: main compiler logic.
- `backends/tofino/bf-p4c/parde/`, `mau/`, `phv/`: parser/deparser, match-action, and PHV allocation subsystems.

## Architecture Notes
- Mid-end transforms frontend IR into a Tofino pipeline IR rooted at `IR::BFN::Pipe`.
- Backend focuses on resource allocation and architecture-specific scheduling across parser/deparser and MAU stages.

## Dependencies (Tests)
- Tests rely on the Tofino assembler and model; see `backends/tofino/bf-p4c/README.md` for setup and required versions.

## Tests & Outputs
- Test targets are backend-specific; see `backends/tofino/` CMake files and `bf-p4c/` CMake/CMakelists for custom targets.
- Idiomatic filtered runs: `ctest --test-dir build -j<N> --output-on-failure -R tofino`.
- Integration inputs and expected outputs live under `testdata/`.

## Navigation Tips
- Start in `bf-p4c/` and follow the pipeline IR (`IR::BFN::*`) and subsystem directories (`parde/`, `mau/`, `phv/`).
