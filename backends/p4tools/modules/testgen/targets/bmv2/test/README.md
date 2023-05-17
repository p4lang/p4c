CMake Files:
=====================
1) P4Tests.cmake - Common test suite to add P4 tests from P4C submodules.
   - Run p4testgen on P4-16 V1Model p4s with the BMv2 target.
2) BMV2...Xfail.cmake - BMv2 xfails for the various BMv2 V1Model back ends.

How to Run tests:
=====================
1) All p4c submodule tests are tagged with 'testgen-p4c-bmv2' label
   - cd build/testgen
   - ctest -R testgen-p4c-bmv2

