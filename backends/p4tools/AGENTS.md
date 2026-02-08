# Repository Guidelines — p4tools

## Purpose
P4Tools is the P4C subproject that bundles tooling for testing and fuzzing P4 programs. It ships two main tools:
- **P4Testgen**: symbolic-execution-based input/output test generation.
- **P4Smith**: random valid P4 program generation.

## Folder Map (Root)
`backends/p4tools/` contains:
- `cmake/`: CMake helper modules for building P4Tools and its tests.
- `common/`: shared C++ infrastructure used by all tools.
  - `compiler/`: P4 compiler passes and midend helpers.
  - `control_plane/`: control-plane modeling utilities.
  - `core/`: core abstractions (targets, Z3 solver, execution state).
  - `lib/`: reusable data structures and utilities.
- `modules/`: tool implementations and their targets.
  - `testgen/`: P4Testgen implementation.
  - `smith/`: P4Smith implementation.
- `p4tools.def`: IR extensions used by P4Tools.
- `CMakeLists.txt`: top-level CMake wiring and module discovery.
- `BUILD.bazel`: Bazel build rules (limited scope).

## Core Concepts
- **IR Extensions**: `p4tools.def` adds custom IR nodes such as `StateVariable`,
  `TaintExpression`, `Extracted_Varbits`, and `ConcolicVariable`. These are
  compiled into the P4C IR via `IR_DEF_FILES`.
- **Shared Infrastructure**: `common/` hosts libraries, solver bindings, and
  compiler helpers used by both tools.
- **Modular Targets**: Both P4Testgen and P4Smith are extensible via target
  subdirectories under `modules/*/targets/`, each providing a `register.h`.

## Build System (CMake)
Top-level CMake behavior:
- `CMakeLists.txt` auto-discovers tool modules under `modules/*`.
- Each module gets a toggle `ENABLE_TOOLS_MODULE_<NAME>`.
- Each tool discovers targets under `modules/<tool>/targets/*` and creates
  `ENABLE_TOOLS_TARGET_<TARGET>` toggles.
- `common/CMakeLists.txt` fetches Z3 via `cmake/Z3.cmake` and exports it with the
  `p4tools-common` library.

P4Testgen specifics:
- `modules/testgen/CMakeLists.txt` fetches `inja` and wires `p4testgen`.
- `linkp4testgen` creates symlinks for the binary and `p4include`.
- Optional gtests are built as `testgen-gtest` when `ENABLE_GTESTS=ON`.

P4Smith specifics:
- `modules/smith/CMakeLists.txt` wires `p4smith` and generates `version.h`.

Build enablement:
- Configure the parent P4C build with `-DENABLE_TEST_TOOLS=ON`.
- Z3 is required; supported versions are constrained (see `README.md`).

## Build System (Bazel)
`BUILD.bazel` provides a limited Bazel build:
- Only `p4testgen` is wired by default.
- `TESTGEN_TARGETS` currently includes only `bmv2`.
- Target registration is generated into `modules/testgen/register.h` via a
  Bazel `genrule`.
If you add a new Testgen target and need Bazel support, extend `TESTGEN_TARGETS`
and the corresponding `filegroup`/`genrule`.

## Extension Points
To add a new target:
1. Create `modules/testgen/targets/<name>/` or `modules/smith/targets/<name>/`.
2. Provide a `CMakeLists.txt` and a `register.h`.
3. Ensure the target registers itself in the generated `register.h`.
4. If Bazel support is required, add it to `BUILD.bazel` (`TESTGEN_TARGETS`).

## Tests & Benchmarks
- `testgen-gtest` is built when `ENABLE_GTESTS=ON`.
- Idiomatic filtered runs:
  `ctest --test-dir build -j<N> --output-on-failure -R testgen`
  `ctest --test-dir build -j<N> --output-on-failure -R smith`
- CTest labels are defined per target under `modules/testgen/targets/*`.
- Benchmarks and plotting scripts live in `modules/testgen/benchmarks/`.

## Key Entry Points
- `README.md`: overview, dependencies, and layout.
- `modules/testgen/README.md`: P4Testgen usage, extensions, and limitations.
- `modules/smith/README.md`: P4Smith usage and extension notes.
- `CMakeLists.txt`: module/target discovery and build wiring.
- `p4tools.def`: IR extensions used across tools.
