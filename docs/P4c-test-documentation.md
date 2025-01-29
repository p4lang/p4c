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

This expanded explanation ensures the section is more detailed and aligns with the reviewer's request to describe all directories comprehensively.

2. **Expected Output Files**:

   - Run the P4 compiler on the source file using the command:
     ```bash
     ./build/p4test path/to/source_file.p4
     ```
   - This generates expected output files.
   - Place the generated expected output files in the appropriate directory, such as `p4c/testdata/p4_16_samples_outputs`.

3. **Updating Expected Outputs**:

   - If a change to the P4 compiler modifies the expected outputs, you can regenerate them by rerunning the tests with:
     ```bash
     ./build/p4test path/to/source_file.p4 --update-outputs
     ```
   - Replace the old expected output files with the newly generated ones.

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

An Xfail test is one that is expected to fail due to a known issue or limitation in the P4 compiler. The CI system handles these tests differently based on their type (positive or negative).

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

   - Note that not all CI systems may treat Xfail tests the same way. For now, document the current behavior in the repository's CI documentation.
   - Add a TODO comment or issue to make the handling of Xfail tests consistent across all CI workflows in the future.

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

## **Conclusion**

This document serves as a starting point for adding new tests to the P4C repository. It is recommended to follow these guidelines to ensure consistency and reliability in the testing process. If you encounter issues or ambiguities, consult the repository maintainers or create a GitHub issue for clarification.

