# p4-tests

This is the common repository for testing p4 programs.

CMake Files:
=====================
1) TestUtils.cmake - Contains macros and functions used to find and add tests/xfails/labels to the test suite.
2) P4Tests.cmake - Common test suite to add p4-tests from p4c submodules.
   - Run p4testgen on p4-16 v1model p4s with bmv2 Target
3) BMV2Xfail.cmake - bmv2 xfails for p4s in p4c submodule

How to Run tests:
=====================
1) All p4c submodule tests are tagged with 'testgen-p4c-bmv2' label
   - cd build/testgen
   - ctest -R testgen-p4c-bmv2

