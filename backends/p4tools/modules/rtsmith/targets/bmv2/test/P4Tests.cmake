# This file defines how to execute Flay on P4 programs. General test utilities.
include(${P4TOOLS_SOURCE_DIR}/cmake/TestUtils.cmake)
# This file defines how we write the tests we generate.
include(${CMAKE_CURRENT_LIST_DIR}/TestTemplate.cmake)


if (TARGET flay)
  # ##################################################################################################
  # TEST PROGRAMS
  # ##################################################################################################
  set(V1_SEARCH_PATTERNS "include.*v1model.p4" "main|common_v1_test")
  # General BMv2 tests supplied by the compiler.
  set(FLAY_CHECKER_P4TESTS_FOR_BMV2
      "${P4C_SOURCE_DIR}/testdata/p4_16_samples/*.p4"
      "${P4C_SOURCE_DIR}/testdata/p4_16_samples/dash/*.p4"
      "${P4C_SOURCE_DIR}/testdata/p4_16_samples/fabric_*/fabric.p4"
      "${P4C_SOURCE_DIR}/testdata/p4_16_samples/omec/*.p4"
      "${P4C_SOURCE_DIR}/testdata/p4_16_samples/pins/*.p4"
      # Custom tests
      "${flay_SOURCE_DIR}/targets/bmv2/test/programs/*.p4"
  )

  p4c_find_tests("${FLAY_CHECKER_P4TESTS_FOR_BMV2}" FLAY_CHECKER_P4_16_BMV2_V1_TESTS INCLUDE "${V1_SEARCH_PATTERNS}" EXCLUDE "")

  # Filter some programs  because they have issues that are not captured with Xfails.
  list(
    REMOVE_ITEM
    FLAY_P4_16_BMV2_V1_TESTS
    # These tests time out and require fixing.
  )

  set (EXTRA_OPTS "--seed 1")

  p4tools_add_tests(
    TESTS
    "${FLAY_CHECKER_P4_16_BMV2_V1_TESTS}"
    TAG
    "rtsmith-checker-bmv2-v1model"
    DRIVER
    ${FLAY_CHECKER_DRIVER}
    TARGET
    "bmv2"
    ARCH
    "v1model"
    TEST_ARGS
    "${EXTRA_OPTS}"
  )

  # Include the list of failing tests.
  include(${CMAKE_CURRENT_LIST_DIR}/BMv2V1ModelXfail.cmake)

endif()

add_test(NAME rtsmith-bmv2-generate
  COMMAND ${RTSMITH_DRIVER} --target bmv2 --arch v1model --seed 1
          --output-dir ${RTSMITH_DIR}/smoke
          ${CMAKE_CURRENT_LIST_DIR}/../../../test/rtsmith-bmv2.p4)
set_tests_properties(rtsmith-bmv2-generate PROPERTIES LABELS "rtsmith" TIMEOUT 60)
add_test(NAME rtsmith-bmv2-cli
  COMMAND "${PYTHON_EXECUTABLE}" ${CMAKE_CURRENT_LIST_DIR}/../../../test/test_cli.py
          ${RTSMITH_DRIVER} ${CMAKE_CURRENT_LIST_DIR}/../../../test/rtsmith-bmv2.p4)
set_tests_properties(rtsmith-bmv2-cli PROPERTIES LABELS "rtsmith" TIMEOUT 60)
if(ENABLE_BMV2 AND HAVE_SIMPLE_SWITCH)
  add_test(NAME rtsmith-bmv2-stf
    COMMAND "${PYTHON_EXECUTABLE}" ${P4C_SOURCE_DIR}/backends/bmv2/run-bmv2-test.py
            ${P4C_SOURCE_DIR} ${CMAKE_CURRENT_LIST_DIR}/../../../test/rtsmith-bmv2.p4
            --test_file ${CMAKE_CURRENT_LIST_DIR}/../../../test/rtsmith-bmv2.stf
            --buildir ${P4C_BINARY_DIR})
  set_tests_properties(rtsmith-bmv2-stf PROPERTIES
    LABELS "rtsmith" TIMEOUT 60 WORKING_DIRECTORY ${P4C_BINARY_DIR})
endif()
if(ENABLE_BMV2 AND HAVE_SIMPLE_SWITCH_GRPC)
  foreach(seed RANGE 1 3)
    add_test(NAME rtsmith-bmv2-replay-${seed}
      COMMAND "${PYTHON_EXECUTABLE}" ${CMAKE_CURRENT_LIST_DIR}/run_test_batch.py
              ${CMAKE_CURRENT_LIST_DIR}/../../../test/rtsmith-bmv2.p4
              --p4rtsmith ${RTSMITH_DRIVER} --build-dir ${P4C_BINARY_DIR} --seed ${seed})
    set_tests_properties(rtsmith-bmv2-replay-${seed} PROPERTIES
      LABELS "rtsmith" TIMEOUT 180)
  endforeach()
endif()
