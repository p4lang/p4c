# Repository Guidelines — bmv2

## Purpose
The BMv2 backend generates code for the Behavioral Model v2 target and supports P4_14 and P4_16 programs for the `v1model.p4` architecture.

## Key Entry Points
- `backends/bmv2/README.md`: backend overview, dependencies, and limitations.
- `backends/bmv2/CMakeLists.txt`: build targets and options.
- `backends/bmv2/`: compiler implementation and helpers.

## Dependencies
- BMv2 behavioral model (see `backends/bmv2/README.md`).
- Python `scapy` for testing.

## Tests & Outputs
- Test targets are backend-specific; see `backends/bmv2/CMakeLists.txt` and any `*.cmake` files for custom targets.
- Idiomatic filtered runs: `ctest --test-dir build -j<N> --output-on-failure -R bmv2`.
- Integration inputs and expected outputs live under `testdata/`.

## Scope & Purpose
This folder implements the BMv2 backend for the P4C compiler. It produces JSON for the BMv2 behavioral model and builds three target-specific compiler binaries:
- `p4c-bm2-ss` for `simple_switch` (v1model).
- `p4c-bm2-psa` for PSA.
- `p4c-bm2-pna` for PNA NIC.

## Directory Layout
Top-level folders and what they own:
- `common/`: shared backend helpers (JSON object model, extern handling, parser/deparser lowering, control flow graph, etc.).
- `simple_switch/`: v1model backend (Simple Switch) entrypoints and midend/backend glue.
- `psa_switch/`: PSA backend entrypoints and midend/backend glue.
- `pna_nic/`: PNA NIC backend entrypoints and midend/backend glue.
- `portable_common/`: common pieces shared by PSA/PNA (portable options, midend, helpers).

Top-level files and scripts:
- `CMakeLists.txt`: build + test wiring for all BMv2 targets, including test suites and XFAIL lists.
- `README.md`: backend overview and unsupported language features.
- `bmv2.def`: IR op definitions specific to BMv2 (e.g., IntMod operation).
- `run-bmv2-test.py`: STF-based test runner.
- `run-bmv2-ptf-test.py`: PTF-based test runner for gRPC-based simple_switch.
- `bmv2stf.py`: STF execution driver and grammar handling.

## Key Entry Points
All three `main.cpp` entrypoints follow the same compilation flow:
1. Parse options (target-specific options class).
2. Run frontend (unless loading from JSON).
3. Serialize P4Runtime info if requested.
4. Run target midend.
5. Run target backend to generate BMv2 JSON output.

Where to look for each target:
- `simple_switch/main.cpp` + `simple_switch/midend.*` + `simple_switch/simpleSwitch.*`
- `psa_switch/main.cpp` + `psa_switch/midend.*` + `psa_switch/psaSwitch.*`
- `pna_nic/main.cpp` + `pna_nic/midend.*` + `pna_nic/pnaNic.*` + `pna_nic/pnaProgramStructure.*`

Common options are defined in `common/options.h`. Target-specific options extend these in:
- `simple_switch/options.h`
- `psa_switch/options.h`
- `portable_common/options.h` (shared by PSA/PNA)

## Build & Test Wiring (CMake)
`CMakeLists.txt`:
- Builds `bmv2backend` (shared library) and `portable-common` (shared PSA/PNA library).
- Builds `p4c-bm2-ss`, `p4c-bm2-psa`, and `p4c-bm2-pna`.
- Adds a `linkbmv2` target to create symlinks expected by legacy test scripts.
- Defines BMv2 test suites, PSA/PNA variants, PTF suites, and XFAIL lists.

## Test Runners
- `run-bmv2-test.py`: compiles and executes STF tests; uses `bmv2stf.py` for STF parsing and runtime.
- `run-bmv2-ptf-test.py`: sets up a namespace environment and runs PTF tests against `simple_switch_grpc`.

## Generated/Derived Artifacts (Gotchas)
- `parsetab.py` and `parser.out` are PLY-generated from the STF grammar. Do not edit manually.
- `__pycache__/` contains runtime bytecode and is not source.
- JSON dump behavior differs subtly between targets; verify the conditions in each `main.cpp` if you rely on `--dump-json` behavior.

## Suggested Reading Order
1. `README.md` for capabilities/limitations.
2. `CMakeLists.txt` for build/test wiring.
3. `common/` for shared backend behavior.
4. Target-specific folder (`simple_switch/`, `psa_switch/`, `pna_nic/`).
