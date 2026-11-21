# P4C Test Guide

## Overview
This document serves as a guide for adding and running tests in the P4C repository. It includes details on test types, where to add new tests, and how to execute them.

## Types of Tests

### Choosing a suite

Place P4₁₄ inputs in `testdata/p4_14/` and backend-independent P4₁₆ regressions
in `testdata/core/`. For backend tests, choose the architecture under `bmv2/`
(v1model, PSA, PNA), `dpdk/` (PSA, PNA), or `ebpf/` (classic, XDP, PSA),
or the `tc/`, `tofino/`, or `ubpf/` suite. A model include is a useful hint;
a frontend regression can use a model without testing its backend.

Use `shared/psa/` or `shared/pna/` when both BMv2 and DPDK should exercise the
same input. Put dedicated Testgen programs in the target's `testgen/` folder.
Runners and output formats can register the same input with different options.
Keep companion packet tests and include files with the program.

A suite's `errors/` subdirectory contains programs that its runner should
reject. Backend rejection does not imply frontend rejection. XFAIL means a
known discrepancy with the expected outcome, not an expected compiler diagnostic.

### Expected output files

Insert `output/` immediately after `testdata/` to find the reference directory
for an input. This rule applies to rejection tests and nested projects too.
Different consumers can check different artifacts in the same directory:
`p4test` checks `foo.p4-stderr`, while DPDK checks `foo.p4.dpdk-stderr`.

#### What Gets Generated
Running the compiler on a test writes reference files into the mirrored directory under `testdata/output/`. The set of files depends on the test type.

A positive test in `testdata/core/foo.p4` produces five files in `testdata/output/core/`:
- `foo.p4`: the program after parsing, printed back to P4.
- `foo-first.p4`: the program near the start of the frontend (the `FrontEndDump` pass).
- `foo-frontend.p4`: the program after the frontend finishes (the `FrontEndLast` pass).
- `foo-midend.p4`: the program after the midend finishes (the `MidEndLast` pass).
- `foo.p4-stderr`: anything the compiler wrote to stderr, such as warnings.

A negative test in `testdata/core/errors/foo.p4` produces two files in `testdata/output/core/errors/`. Compilation stops at the error, so the frontend and midend dumps are not written:
- `foo.p4`: the program after parsing, printed back to P4.
- `foo.p4-stderr`: the expected error and warning messages.

A test targeting BMv2 (file name ending in `-bmv2.p4`) also generates `foo.p4info.txtpb` and `foo.entries.txtpb` when the program produces P4Runtime information.

Commit every generated file along with the test source. All of them are compared on each run, and any mismatch fails the test.

### XFAIL Tests
XFAIL tests are used when a test is currently failing due to a known compiler issue. For example, a positive test may fail due to an unexpected compile-time error, or a negative test may not detect an intended error. These tests are temporarily marked as XFAIL to track the issue while preventing the test suite from failing. Once the bug is resolved, the XFAIL status should be removed.

#### Source File Location
- Various locations within `testdata/`

## Running Tests

You can run tests using `ctest`, which is the recommended approach moving forward:

- **Run all tests:**  
  `ctest --output-on-failure`

- **Run a subset of tests (filter by name):**  
  `ctest --output-on-failure -R <pattern>`

- **Rerun only previously failed tests:**  
  `ctest --output-on-failure --rerun-failed`

- **Run a specific test (by full or partial test name):**  
  `ctest --output-on-failure -R '<test_name>'`

- **Tests for specific backends:**  
  `ctest --output-on-failure -R bmv2`  
  `ctest --output-on-failure -R ebpf`

## Adding New Tests

1. Add the P4 file to the suite that checks its behavior. Use a descriptive name or issue number.
2. Run `cmake -B build` to discover the new input. Use `ctest --test-dir build -N -R <name>` to check which runners selected it.
3. Run `P4TEST_REPLACE=1 ctest --test-dir build -R <name>` to generate the references. Inspect the changes and rerun without `P4TEST_REPLACE`.
4. Commit the input, companion files, and references together.

Out-of-tree extensions that selected `p4_16_samples/*.p4` should select their
suite directories explicitly. Do not recursively glob all of `testdata/`:
`output/` also contains P4 files, and multi-file programs have include-only files.

## Continuous Integration
When a pull request is created, the test suite runs automatically using `ctest`. Tests are executed using a recently built version of `simple_switch` from the `p4lang/behavioral-model` repository.

For more details, refer to:
- [P4C repository](https://github.com/p4lang/p4c)
- [P4 Behavioral Model Docker builds](https://hub.docker.com/r/p4lang/behavioral-model/builds)

