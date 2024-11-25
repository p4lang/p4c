# General test utilities.
include(${P4TOOLS_SOURCE_DIR}/cmake/TestUtils.cmake)
# This file defines how we write the tests we generate.
include(${CMAKE_CURRENT_LIST_DIR}/TestTemplate.cmake)

set (TOFINO_SOURCE_DIR ${P4C_SOURCE_DIR}/backends/tofino)

# # # ##########################################################################
# TEST SUITES
# # # ##########################################################################
# Check for PTF
find_program(PTF ptf PATHS ${CMAKE_INSTALL_PREFIX}/bin)

if(PTF)
  message(STATUS "ptf for ${device} found at ${PTF}.")
else()
  message(WARNING "PTF tests need ptf for ${device}.\nLooked in ${CMAKE_INSTALL_PREFIX}/bin.")
endif()

set(
  BF_SWITCHD_SEARCH_PATHS
  ${BFN_P4C_SOURCE_DIR}/../install/bin
  ${CMAKE_INSTALL_PREFIX}/bin
)


set(
  BF_SWITCH_INC_PATH
  ${TOFINO_SOURCE_DIR}/p4-tests/p4_16/switch_16/p4src/shared/
)

# check for bf_switchd
find_program(BF_SWITCHD bf_switchd PATHS ${BF_SWITCHD_SEARCH_PATHS} NO_DEFAULT_PATH)

if(BF_SWITCHD)
  message(STATUS "bf-switchd for ${device} found at ${BF_SWITCHD}.")
else()
  message(WARNING "PTF tests need bf-switchd for ${device}.\nLooked in ${BF_SWITCHD_SEARCH_PATHS}.")
endif()

# check for tofino-model
set(
  HARLYN_MODEL_SEARCH_PATHS
  ${BFN_P4C_SOURCE_DIR}/../install/bin
  ${CMAKE_INSTALL_PREFIX}/bin
)

find_program(HARLYN_MODEL tofino-model PATHS ${HARLYN_MODEL_SEARCH_PATHS} NO_DEFAULT_PATH)

if(HARLYN_MODEL)
  message(STATUS "tofino-model for ${device} found at ${HARLYN_MODEL}.")
else()
  message(WARNING "PTF tests need tofino-model for ${device}.\nLooked in ${HARLYN_MODEL_SEARCH_PATHS}.")
endif()

# # # ##########################################################################
# BF-P4C TNA P4-PROGRAMS TESTS
# # # ##########################################################################
file(
  GLOB
  P4TOOLS_TESTGEN_TOFINO
  # Compiler tests.
  ${TOFINO_SOURCE_DIR}/p4-tests/p4-programs/p4_16_programs/*/*.p4
  ${TOFINO_SOURCE_DIR}/p4-tests/p4-programs/internal_p4_16/*/*.p4
  ${TOFINO_SOURCE_DIR}/p4-tests/p4_16/compile_only/*.p4
  ${TOFINO_SOURCE_DIR}/p4-tests/p4_16/switch_16/p4src/switch-tofino/*.p4
  ${TOFINO_SOURCE_DIR}/p4-tests/p4_16/switch_16/p4src/switch-tofino2/*.p4
  ${TOFINO_SOURCE_DIR}/p4-tests/p4_16/stf/*.p4
)

# Some p4s are compiler Xfails - excluding them here.
set(
  P4TOOLS_TESTGEN_TOFINO_EXCLUDES
  # ${TOFINO_SOURCE_DIR}/p4-tests/p4_16/compile_only/atcam_match_wide1-neg.p4
  # ${TOFINO_SOURCE_DIR}/p4-tests/p4_16/compile_only/dkm_invalid.p4
  # ${TOFINO_SOURCE_DIR}/p4-tests/p4_16/compile_only/multi-constraint.p4
  # ${TOFINO_SOURCE_DIR}/p4-tests/p4_16/compile_only/neg_test_1_lpf_constant_param.p4
  # ${TOFINO_SOURCE_DIR}/p4-tests/p4_16/compile_only/too_many_ternary_match_key_bits.p4
  # ${TOFINO_SOURCE_DIR}/p4-tests/p4_16/compile_only/checksum_neg_test1.p4
  # ${TOFINO_SOURCE_DIR}/p4-tests/p4_16/compile_only/checksum_neg_test2.p4
  # ${TOFINO_SOURCE_DIR}/p4-tests/p4_16/compile_only/checksum_neg_test3.p4
  # # Set as Tofino Errors.
  # ${TOFINO_SOURCE_DIR}/p4-tests/p4_16/stf/parser_multi_write_2.p4
  # ${TOFINO_SOURCE_DIR}/p4-tests/p4_16/stf/parser_multi_write_8.p4
  # ${TOFINO_SOURCE_DIR}/p4-tests/p4_16/stf/parser_multi_write_checksum_deposit_2.p4
  # ${TOFINO_SOURCE_DIR}/p4-tests/p4_16/stf/parser_multi_write_checksum_verify_2.p4
  # Infinite parser loop. TODO: Fix handling of infinite parser loops.
  # This test takes extremely long to compile.
  # This test has very complex conditions, causing the solver to time out.
)
# Some p4s are included in earlier folders (even though they are different) - excluding them here.
# If they are needed, we have to add them explicitly with a different test name.
set(
  P4TOOLS_TESTGEN_TOFINO_DUPLICATES
  ${TOFINO_SOURCE_DIR}/p4-tests/p4_16/stf/match_key_slices.p4
)
list(REMOVE_ITEM P4TOOLS_TESTGEN_TOFINO ${P4TOOLS_TESTGEN_TOFINO_DUPLICATES} ${P4TOOLS_TESTGEN_TOFINO_EXCLUDES})

set(TNA_SEARCH_PATTERNS "include.*tna.p4" "main")
p4c_find_tests("${P4TOOLS_TESTGEN_TOFINO}" P4TOOLS_TESTGEN_TOFINO_TNA INCLUDE "${TNA_SEARCH_PATTERNS}" EXCLUDE "${P4TOOLS_TESTGEN_TOFINO_EXCLUDES}")

# Custom tests, not subject to deduplication.
file(
  GLOB
  P4TOOLS_TESTGEN_TOFINO_CUSTOM
  ${CMAKE_CURRENT_LIST_DIR}/p4-programs/*.p4
  ${CMAKE_CURRENT_LIST_DIR}/p4-programs/tna_simple_switch/tna_simple_switch.p4
  ${CMAKE_CURRENT_LIST_DIR}/p4-programs/tofino1_only/*.p4
)

set(
  P4TOOLS_TESTGEN_TOFINO_TNA
  ${P4TOOLS_TESTGEN_TOFINO_TNA}
  ${P4TOOLS_TESTGEN_TOFINO_CUSTOM}
)

set(T2NA_SEARCH_PATTERNS "include.*t2na.p4" "main")
p4c_find_tests("${P4TOOLS_TESTGEN_TOFINO}" P4TOOLS_TESTGEN_TOFINO_T2NA INCLUDE "${T2NA_SEARCH_PATTERNS}" EXCLUDE "${P4TOOLS_TESTGEN_TOFINO_EXCLUDES}")

# Custom tests, not subject to deduplication.
file(
  GLOB
  P4TOOLS_TESTGEN_TOFINO_T2NA_CUSTOM
  ${CMAKE_CURRENT_LIST_DIR}/p4-programs/*.p4
  ${CMAKE_CURRENT_LIST_DIR}/p4-programs/tna_simple_switch/tna_simple_switch.p4
  ${CMAKE_CURRENT_LIST_DIR}/p4-programs/tofino2_only/*.p4
)

set(
  P4TOOLS_TESTGEN_TOFINO_T2NA
  ${P4TOOLS_TESTGEN_TOFINO_T2NA}
  ${P4TOOLS_TESTGEN_TOFINO_T2NA_CUSTOM}
)

# # # ##########################################################################
# STF TESTS
# # # ##########################################################################
set(
  P4TOOLS_TESTGEN_TOFINO_INC_PATH
  ${TOFINO_SOURCE_DIR}/p4-tests/p4-programs/p4_16_programs/
)
set(
  P4TOOLS_TESTGEN_TOFINO_P416_INC_PATH
  ${TOFINO_SOURCE_DIR}/p4-tests/p4_16/includes
)

set(
  P4TOOLS_TESTGEN_TOFINO_INTERNAL_TESTS_INC_PATH
  ${TOFINO_SOURCE_DIR}/p4-tests/p4_16/stf/include/
)
set(
  P4TOOLS_TESTGEN_TOFINO_SWITCH_INC_PATH
  ${TOFINO_SOURCE_DIR}/p4-tests/p4_16/switch_16/p4src/shared
)

# Test settings.
set(P416INCLUDES "-I${P4C_SOURCE_DIR}/backends/tofino/bf-p4c/p4include -I${P4C_SOURCE_DIR}/p4include -I${P4TOOLS_TESTGEN_TOFINO_INC_PATH} -I${P4TOOLS_TESTGEN_TOFINO_P416_INC_PATH} -I${P4TOOLS_TESTGEN_TOFINO_INTERNAL_TESTS_INC_PATH} -I${P4TOOLS_TESTGEN_TOFINO_SWITCH_INC_PATH}")
if(NOT NIGHTLY)
  set(EXTRA_OPTS "${P416INCLUDES} --print-traces --strict --seed 1000 --max-tests 100")
else()
  set(EXTRA_OPTS "${P416INCLUDES} --print-traces --strict --max-tests 500")
endif()

# # # ##########################################################################
# STF TESTS
# # # ##########################################################################
p4tools_add_tests(
  TESTS "${P4TOOLS_TESTGEN_TOFINO_TNA}"
  TAG "testgen-tofino" DRIVER ${P4TESTGEN_DRIVER}
  TARGET "tofino" ARCH "tna" CTEST_P4C_ARGS "${P416INCLUDES} --disable-power-check --disable-parse-depth-limit" RUN_STF TEST_ARGS "-D__TARGET_TOFINO__=1 --test-backend STF --port-ranges 0:63 ${EXTRA_OPTS}"
)

# include(${CMAKE_CURRENT_LIST_DIR}/TofinoXfail.cmake)

set (P4TOOLS_TESTGEN_TOFINO_T2NA_STF ${P4TOOLS_TESTGEN_TOFINO_T2NA})
list(REMOVE_ITEM P4TOOLS_TESTGEN_TOFINO_T2NA_STF
  # This test times out when running on STF.
)
p4tools_add_tests(
  TESTS "${P4TOOLS_TESTGEN_TOFINO_T2NA_STF}"
  TAG "testgen-tofino2" DRIVER ${P4TESTGEN_DRIVER}
  TARGET "tofino2" ARCH "t2na" CTEST_P4C_ARGS "${P416INCLUDES} --disable-power-check --disable-parse-depth-limit" RUN_STF TEST_ARGS "-D__TARGET_TOFINO__=2 --test-backend STF --port-ranges 8:71 ${EXTRA_OPTS}"
)
# include(${CMAKE_CURRENT_LIST_DIR}/Tofino2Xfail.cmake)

# # # ##########################################################################
# PTF TESTS
# # # ##########################################################################

p4tools_add_tests(
  TESTS "${P4TOOLS_TESTGEN_TOFINO_TNA}"
  TAG "testgen-tofino-ptf" DRIVER ${P4TESTGEN_DRIVER}
  TARGET "tofino" ARCH "tna" CTEST_P4C_ARGS "${P416INCLUDES} --disable-power-check --disable-parse-depth-limit" RUN_PTF TEST_ARGS "-D__TARGET_TOFINO__=1 -I${P4C_SOURCE_DIR}/p4_16/includes  --test-backend PTF --port-ranges 0:15 ${EXTRA_OPTS}"
)
# include(${CMAKE_CURRENT_LIST_DIR}/TofinoPTFXfail.cmake)

p4tools_add_tests(
  TESTS "${P4TOOLS_TESTGEN_TOFINO_T2NA}"
  TAG "testgen-tofino2-ptf" DRIVER ${P4TESTGEN_DRIVER}
  TARGET "tofino2" ARCH "t2na" CTEST_P4C_ARGS "${P416INCLUDES} --disable-power-check --disable-parse-depth-limit" RUN_PTF TEST_ARGS "-D__TARGET_TOFINO__=2 --test-backend PTF --port-ranges 8:15 ${EXTRA_OPTS}"
)
# include(${CMAKE_CURRENT_LIST_DIR}/Tofino2PTFXfail.cmake)


# # # ##########################################################################
# TEST PROPERTIES
# # # ##########################################################################

# These are pathologically slow tests that need extra time.
# set_tests_properties("testgen-tofino2-ptf/switch_tofino2_y2.p4" PROPERTIES TIMEOUT 6000)
# set_tests_properties("testgen-tofino2/switch_tofino2_y2.p4" PROPERTIES TIMEOUT 6000)
# set_tests_properties("testgen-tofino2-ptf/switch_tofino2_y1_mod.p4" PROPERTIES TIMEOUT 6000)
# set_tests_properties("testgen-tofino2/switch_tofino2_y1_mod.p4" PROPERTIES TIMEOUT 6000)


# set_tests_properties("testgen-tofino-ptf/switch_tofino_x2.p4" PROPERTIES TIMEOUT 6000)
# set_tests_properties("testgen-tofino/switch_tofino_x2.p4" PROPERTIES TIMEOUT 6000)
# set_tests_properties("testgen-tofino/switch_tofino_x1_mod.p4" PROPERTIES TIMEOUT 6000)

# set_tests_properties("testgen-tofino2/tna_alpmV2.p4" PROPERTIES TIMEOUT 3000)
# set_tests_properties("testgen-tofino2/tna_ipv4_alpm.p4" PROPERTIES TIMEOUT 3000)


# # # ##########################################################################
# UNSTABLE TESTS
# # # ##########################################################################
# p4c_add_test_label("testgen-tofino2-ptf" "UNSTABLE" "switch_tofino2_y1_mod.p4")


# # # ##########################################################################
# ADD NIGHTLY LABEL TO GROUP OF TESTS
# # # ##########################################################################
# macro(p4tools_add_to_nightly p4ProgramPaths testLabel)
#   message("P4 programs added to Nightly:")
#   foreach(programPath ${p4ProgramPaths})
#     get_filename_component(programFileName ${programPath} NAME)
#     p4c_add_test_label("${testLabel}" "NIGHTLY" "${programFileName}")
#     message(">>>NIGHTLY: ${testLabel}/${programFileName}")
#   endforeach()
# endmacro(p4tools_add_to_nightly)

# p4tools_add_to_nightly("${P4TOOLS_TESTGEN_TOFINO_TNA}" "testgen-tofino")
