# Repository Guidelines — p4test

## Purpose
`p4test` is a backend-agnostic testing tool that exercises the P4 compiler front-end and mid-end. It underpins many test cases in the top-level `tests/` directory.

## Key Entry Points
- `backends/p4test/README.md`: usage and intent.
- `backends/p4test/CMakeLists.txt`: build targets and test variants.
- `backends/p4test/`: implementation and supporting passes.

## Tests & Variants
- Variants and drivers are defined in `backends/p4test/CMakeLists.txt`.
- Idiomatic filtered runs: `ctest --test-dir build -j<N> --output-on-failure -R p4test`.
- The top-level `tests/` directory references `p4test` for many suites.
- Integration inputs and expected outputs live under `testdata/`.

## Navigation Tips
- Start in `backends/p4test/` and follow variant setup in `CMakeLists.txt` to see how tests are wired.
