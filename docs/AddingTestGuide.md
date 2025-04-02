# P4C Test Guide

## Overview
This guide explains how to add, run, and manage tests in the P4C repository.

## Types of Tests

### Positive Tests
Positive tests are standard tests where the compiler should not fail due to compile-time errors. The expected output should match the actual output for the test to pass.

#### Source File Location
- `testdata/p4_16_samples/`

### Negative Tests
Negative tests check if the compiler correctly catches syntax or semantic errors. These tests are placed in directories with "errors" in their names, and the expected output includes error messages.

#### Source File Location
- `testdata/p4_16_errors/`

### Backend-Specific Tests
Some test programs exist in backend-specific directories, such as eBPF tests.

#### Source File Location
- `backends/ebpf/tests/`

### Expected Output Files
Tests often have corresponding expected output files in directories suffixed with `_outputs`. Any differences between expected and actual output will cause a test failure.

#### Source File Location
- `testdata/p4_16_samples_outputs/`
- `testdata/p4_16_errors_outputs/`

### XFAIL Tests
These are tests that are expected to fail due to known issues. They help track compiler bugs without failing the entire test suite.

#### Source File Location
- Various locations within `testdata/`

## Running Tests

- To run all tests: `make check -j3`
- To run a subset of tests: `make check-PATTERN` (e.g., `make check-p4`)
- To rerun failed tests: `make recheck`
- To run a single test: `ctest --output-on-failure -R '<test>'`
- Tests for specific backends:
  - `make check-bmv2`
  - `make check-ebpf`

## Adding New Tests

1. Add the new `.p4` test file to the appropriate directory (`testdata/p4_16_samples/` for standard tests).
2. Generate reference outputs:
   - For frontend tests: `../backends/p4test/run-p4-sample.py . -f ../testdata/p4_16_samples/some_name.p4`
   - For bmv2 tests: `../backends/bmv2/run-bmv2-test.py`
3. Commit the test and reference output files.

## Continuous Integration
When a pull request is created, the test suite runs automatically using `make check`. Tests are executed using a recently built version of `simple_switch` from the `p4lang/behavioral-model` repository.

For more details, refer to:
- [P4C repository](https://github.com/p4lang/p4c)
- [P4 Behavioral Model Docker builds](https://hub.docker.com/r/p4lang/behavioral-model/builds)

