# P4C Test Guide

## Overview
This document serves as a guide for adding and running tests in the P4C repository. It includes details on test types, where to add new tests, and how to execute them.

## Types of Tests

### Positive Tests
Positive tests are standard tests where the compiler should not fail due to compile-time errors. The expected output should match the actual output for the test to pass. The compiler should generate an output file with no errors.

#### Source File Location
- `testdata/p4_16_samples/`

### Negative Tests
Negative tests check if the compiler correctly catches syntax or semantic errors. These tests are placed in directories with "errors" in their names, and the expected output includes error messages.

#### Source File Location
- `testdata/p4_16_errors/`
- `testdata/p4_16_bmv_errors/`
- `testdata/p4_16_dpdk_errors/`
- `testdata/p4_16_ebpf_errors/`
- `testdata/p4_16_pna_errors/`
- `testdata/p4_16_psa_errors/`

### Backend-Specific Tests
Some test programs exist in backend-specific directories, such as eBPF and uBPF tests.

#### Source File Location
- `backends/ebpf/tests/`
- `backends/ubpf/tests/`

> **Note:** All new tests must be added in the `testdata` directory unless there is a strong reason to add them somewhere else.

### Expected Output Files
Tests often have corresponding expected output files in directories suffixed with `_outputs`. Any differences between expected and actual output will cause a test failure.

#### Source File Location
- `testdata/p4_16_samples_outputs/`
- `testdata/p4_16_errors_outputs/`

Similarly, for other error output files, you can check:
- `testdata/p4_16_pattern_errors_outputs/`

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

1. Add the new `.p4` test file to the appropriate directory (`testdata/p4_16_samples/` for standard tests).

2. Follow file naming conventions:
   - Tests targeting BMv2 should end with `-bmv2.p4`.
   - Tests targeting eBPF should end with `-ebpf.p4`.
   - General frontend tests should use descriptive names that reflect the feature being tested.

3. Generate reference outputs:
   - For frontend tests:  
     `../backends/p4test/run-p4-sample.py . -f ../testdata/p4_16_samples/some_name.p4`
   - For BMv2 tests:  
     `../backends/bmv2/run-bmv2-test.py`

4. Commit the test and reference output files.

## Continuous Integration
When a pull request is created, the test suite runs automatically using `ctest`. Tests are executed using a recently built version of `simple_switch` from the `p4lang/behavioral-model` repository.

For more details, refer to:
- [P4C repository](https://github.com/p4lang/p4c)
- [P4 Behavioral Model Docker builds](https://hub.docker.com/r/p4lang/behavioral-model/builds)

