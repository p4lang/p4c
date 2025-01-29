# **Adding New Tests to the P4C Repository**

**This document provides a guide for developers who want to add new tests to the P4C repository. It explains where to place test files, how to manage expected output files, and how to handle tests that are expected to fail (Xfail).**

---





## **1. Adding "Positive" Tests**

Positive tests are those where the P4 compiler should process the input without any errors.

### Steps for Adding Positive Tests

1. **Source File Location**:

   - Positive tests are located in multiple directories based on the purpose and the backend/tool being tested. Below is a description of the primary directories under `p4c/testdata` and their purposes:

     - **`p4_16_samples`**: Contains general positive tests for the P4\_16 language.
     - **`p4_16_samples_outputs`**: Stores the expected outputs for the tests in `p4_16_samples`.
     - **`p4_14_samples`**: Contains tests specific to the older P4\_14 language version.
     - **`p4_14_samples_outputs`**: Stores the expected outputs for tests in `p4_14_samples`.
     - **`bmv2`**: Holds tests targeting the BMv2 backend, including packet-processing logic and examples.
     - **`psa`**: Includes tests for the PSA (Portable Switch Architecture) target.
     - **`ebpf`**: Contains tests designed for the eBPF backend.
     - **`xdp`**: Stores tests related to XDP programs using eBPF.
     - **`arch`**: Contains architecture-specific tests, covering various P4 architectures.
     - **`tools`**: Includes tests for tools provided in the P4C repository, such as `p4test` and `p4c-bm2-ss`.
     - **`error_samples`**: Contains tests that verify the compiler's behavior when encountering erroneous P4 programs.
     - **`stf_samples`**: Stores `.stf` (Switch Test Framework) files associated with P4 programs, defining packet inputs/outputs for testing.
     - **`stress_tests`**: Contains larger or more computationally intensive tests to stress-test the compiler.

   - Make sure the file you are adding is placed in the appropriate directory based on the purpose and backend/tool it is testing.
   - Ensure the file is named descriptively to reflect its purpose (e.g., `example_positive_test.p4`).

---



2. **Expected Output Files**:

2. **File Naming Conventions**:

   - For **BMv2-specific tests** with packets, the source file name must match the pattern `*-bmv2.p4`.
  - For **v1model architecture** tests: `*-bmv2-v1model.p4`.

  - For **PSA architecture** tests: `*-bmv2-psa.p4`.
  
   - Additional file name patterns:
     - For eBPF-specific tests: `*_ebpf.p4` or `*_ebpf-kernel.p4`.
     - For graph backend tests: `*graph*.p4`.
     - For UBPF-specific tests: `*_ubpf.p4`.

     - Other specialized tests under subdirectories such as:
       - `p4tc_samples/` for TC tests.
       - `fabric_*/` and `omec/` for Fabric and OMEC tests.
       - `switch_*/` for specific switch samples.

   These naming conventions ensure the CMake configuration files correctly associate test programs with their respective backends or features.


3. **Expected Output Files**:

   - Run the P4 compiler on the source file to verify the output:
     ```bash
     ./build/p4test path/to/source_file.p4
     ```

   - If you need to regenerate expected output files for all test cases, use:
     ```bash
     P4TEST_REPLACE=1 make check
     ```
   - However, this updates outputs for all tests, so use it with caution.

4. **Updating Expected Outputs**:

   - If a change to the P4 compiler modifies the expected outputs, you can regenerate them by running:
     ```bash
     P4TEST_REPLACE=1 ./build/p4test path/to/source_file.p4
     ```
   - This will update the expected output file for the specified test.
   - If you need to regenerate expected outputs for all test cases, use:
     ```bash
     P4TEST_REPLACE=1 make check
     ```
   - Be cautious when using this command, as it updates outputs for all tests.

### Verifying the Test

- Use the CI system or run the test locally to verify that it passes as expected.

---

## **2. Adding "Negative" Tests**

Negative tests are those where the P4 compiler is expected to catch a syntax or semantic error in the input source file.

### Steps for Adding Negative Tests

1. **Source File Location**:

   - Place the source file in the `p4c/testdata/p4_16_errors` directory.
   - Name the file appropriately to describe the expected error (e.g., `example_negative_test.p4`).

2. **Expected Output Files**:

   - Run the P4 compiler on the source file to generate the error output:
     ```bash
     ./build/p4test path/to/source_file.p4
     ```
   - Save the generated error output in the `p4c/testdata/p4_16_errors_outputs` directory with a matching file name.

3. **Verifying the Test**:

   - Run the test locally or through CI to confirm that the compiler generates the expected error.

---

## **3. Handling Different Backends and Tools**

 



- **Different Backends**: The P4C repository supports multiple backends like BMv2, PSA, etc. Each backend may require tests to be placed in specific directories and may generate backend-specific outputs. 

- **Tools**: Tools like `p4testgen` may introduce additional test workflows and configurations.

- **Tests with STF Files**: Some P4 programs include `.stf` (Switch Test Framework) files, which define expected packet inputs and outputs. Ensure these are placed correctly alongside the P4 source files.

- **BMv2 Tests**: For tests specifically targeting the BMv2 backend, file names should match the pattern `*-bmv2.p4`. Identify the file controlling these patterns in the repository to document its location for future test writers.

Refer to the repository documentation and existing test examples to understand these variations better.

---

## **4. Marking Tests as "Expected to Fail" (Xfail)**

An XFail is a test that should pass (by the language spec and architecture), but currently fails due to known problems in the compiler.




### Marking a Test as Xfail

1. **Adding the Xfail Mark**:

   - Modify the test metadata to indicate that it is expected to fail.
   - For example, you might add an `XFAIL` tag in the test description or a specific configuration file.

2. **Behavior of Xfail Tests**:

   - For **positive tests**:
     - If the test passes (i.e., the compiler does not fail), the overall CI run will fail.
   - For **negative tests**:
     - If the test fails (i.e., the compiler generates an error), the overall CI run will fail.

3. **Maintaining Consistency**:

   - Note that different p4c backends may treat Xfail tests differently. **TODO**: Investigate and document how each p4c backend handles Xfail tests to ensure consistency across the project.
   
   - **TODO**: Document which p4c backend tests treat Xfail as described above versus those that treat Xfail differently.





---


## **5. Running Tests**

Tests in the P4C repository can be executed in two main ways:

1. **Using Bash Scripts**:

   - Each test script typically ends with `.test`.
   - Run them directly from the command line, e.g.,
     ```bash
     ./test_script_name.test
     ```

2. **Using CTest**:

   - CTest provides an alternative way to execute tests, often integrated with the CI pipeline.
   - Example command:
     ```bash
     ctest -R test_name
     ```

### Differences Between Bash Scripts and CTest


"**Options Available**: CTest may offer additional or different options compared to Bash scripts. To see the full list of available options, you can run the following command in your terminal:

```bash
ctest --help
```

This will display all the commands and their descriptions that you can use with CTest. For a more detailed explanation of each option and how to use them, refer to the official documentation [here](https://cmake.org/cmake/help/latest/manual/ctest.1.html). This documentation will provide in-depth guidance on all the features and configurations available."

- **Behavior**: Some CI environments may prefer one method over the other for consistency. Use CTest if you want to leverage its filtering and summary capabilities.
- **Recommendation**: Document the preferred approach for specific use cases in the repository README or this guide.

---


## P4 Architecture Support Across Backends

### PSA is an Architecture, Not a Target
The PSA (Portable Switch Architecture) is a P4 architecture, not a target. Different backends implement different architectures depending on their intended use cases.

### Backend Support for P4 Architectures
The table below summarizes which architectures are supported by different P4 backends:

| Backend  | v1model | PSA | PNA |
|----------|--------|-----|-----|
| BMv2     | ✅ (full) | ⚠️ (partial) | ⚠️ (partial) |
| DPDK     | ❌ | ✅ | ✅ |
| eBPF     | ❌ | ✅ | ❌ |
| UBPF     | ❌ | ✅ | ❌ |
| P4Tools  | ✅ | ✅ | ❌ |

**Legend:**
- ✅ = Fully supported  
- ⚠️ = Partially supported  
- ❌ = Not supported  

### Notes on Backend Implementations
- **BMv2**: Primarily supports v1model but has partial implementations for PSA and PNA.  
- **DPDK**: Implements PSA and PNA but does not support v1model.  
- **eBPF & UBPF**: Only implement PSA.  
- **P4Tools**: Has support for both v1model and PSA, as indicated by test files. 

## **Conclusion**

This document serves as a starting point for adding new tests to the P4C repository. It is recommended to follow these guidelines to ensure consistency and reliability in the testing process. If you encounter issues or ambiguities, consult the repository maintainers or create a GitHub issue for clarification.

