<!--
SPDX-FileCopyrightText: 2023 The P4 Language Consortium

SPDX-License-Identifier: Apache-2.0
-->

<!-- 
Documentation Inclusion:
This README is integrated as a subsection of the "P4Testgen" page in the P4 compiler documentation.

Refer to the specific section here: [P4Testgen BMv2 target tests - Subsection](https://p4lang.github.io/p4c/p4testgen.html#p4testgen-bmv2-target-tests)
-->
# P4Testgen BMv2 target tests

## CMake Files

The integration suite and its CMake files live in `testdata/bmv2/testgen/`.

+ `Testgen.cmake` registers the programs.
  + Run p4testgen on P4-16 V1Model p4s with the BMv2 target.
+ BMV2...Xfail.cmake - BMv2 xfails for the various BMv2 V1Model back ends.

## How to Run tests

+ All P4C submodule tests are tagged with 'testgen-p4c-bmv2' label

```bash
ctest --test-dir build -R testgen-p4c-bmv2
```
