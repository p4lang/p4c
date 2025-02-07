####### Tofino assembler -- tests
cmake_minimum_required (VERSION 3.16.3 FATAL_ERROR)

project (BFASM_TEST)

set (CMAKE_MODULE_PATH ${BFN_P4C_SOURCE_DIR}/p4c/cmake)

MESSAGE("-- Adding bf-asm test suite")

# common test utils
#include(../../p4-tests/TestUtils.cmake)

if (ENABLE_TEST_ISOLATION)
  get_filename_component(P4C_RUNTEST ../../p4-tests/internal/runtest-isolated REALPATH)
else()
  get_filename_component(P4C_RUNTEST ../../p4-tests/runtest REALPATH)
endif()


# extra test args can be passed as unamed arguments
function(bfas_add_test_with_args device toolsdevice alias bfafile test_args cmake_args)
  # create the test
  MESSAGE("Invoking: p4c_add_test_with_args (${device} ${P4C_RUNTEST} FALSE ${alias} ${bfafile} \"${test_args}\" \"-${device} ${cmake_args}\")")
  MESSAGE("#macro(p4c_add_test_with_args tag driver isXfail alias p4test test_args cmake_args)")
  #macro(p4c_add_test_with_args tag driver isXfail alias p4test test_args cmake_args)
  p4c_add_test_with_args (${device} ${P4C_RUNTEST} FALSE ${alias}
      ${bfafile} "${test_args}" "-${device} ${cmake_args}")
  if (PTF_REQUIREMENTS_MET)
    set(__havePTF 0)
    string (REGEX REPLACE ".bfa$" ".stf" __stffile ${bfafile})
    if (ENABLE_STF2PTF AND NOT ${__havePTF} AND EXISTS ${BFASM_TEST_SOURCE_DIR}/${__stffile})
      # Also add as PTF test the STF
      # MESSAGE(STATUS "STF2PTF: Generating ${P4C_BINARY_DIR}/${device}/${__ptffile}/test.py")
      set(__havePTF 1)
    endif()
    if (${__havePTF})
      p4c_test_set_name(__testname ${device} ${alias})
      set_ptf_test_locks(${__testname})
      p4c_add_test_label(${device} "ptf" ${alias})
    endif()
  endif() # PTF_REQUIREMENTS_MET
  if (HARLYN_STF_${toolsdevice})
    string (REGEX REPLACE ".bfa$" ".stf" __stffile ${bfafile})
    if (EXISTS ${BFASM_TEST_SOURCE_DIR}/${__stffile})
      p4c_add_test_label(${device} "stf" ${alias})
    endif()
  endif(HARLYN_STF_${toolsdevice})
endfunction(bfas_add_test_with_args)

# extra test args can be passed as unamed arguments
#macro(p4c_add_bf_backend_tests device toolsdevice arch label tests)
#endmacro(p4c_add_bf_backend_tests)
macro(bfas_add_tests device toolsdevice arch label tests)
  set (_testExtraArgs "${ARGN}")
  # do not add the device directly to _testExtraArgs
  # this is used later to add other tests for multiple configurations.
  # set (_testExtraArgs "${_testExtraArgs} -${device}")

  # if STF is not found, disable all stf tests
  if (NOT HARLYN_STF_${toolsdevice})
    set (_testExtraArgs "${_testExtraArgs} -norun")
  endif()

  if (PTF_REQUIREMENTS_MET)
    set (_testExtraArgs "${_testExtraArgs} -ptf")
    if (ENABLE_STF2PTF)
      if ( "${device}" STREQUAL "tofino")
        set (_testExtraArgs "${_testExtraArgs} -stf2ptf")
      endif()
    endif()
  endif()

  # If label is not empty, add it to the tests
  foreach (ts "${tests}")
    MESSAGE("Processing ${ts}")
    file (GLOB __testfiles RELATIVE ${BFASM_SOURCE_DIR} ${ts})
    foreach (__p4file ${__testfiles})
      bfas_add_backend_test_and_label(${device} ${toolsdevice} ${__p4file} ${__p4file} "${label}"
      "${_testExtraArgs}" "")
    endforeach() # __p4file
  endforeach()
endmacro(bfas_add_tests)

macro(bfas_add_backend_test_and_label device toolsdevice alias bfa_file label test_args cmake_args)
  bfas_add_test_with_args(${device} ${toolsdevice} ${alias} ${bfa_file} "${test_args}" "${cmake_args}")
  p4c_add_test_label(${device} "${label}" ${alias})
endmacro(bfas_add_backend_test_and_label )


if (ENABLE_TESTING)
  # Replace P4C source/binary paths temporarily to override the defaults used by p4c_add_test_with_args
  set(P4C_SOURCE_DIR_ORIG ${P4C_SOURCE_DIR})
  set(P4C_SOURCE_DIR ${BFASM_SOURCE_DIR})
  set(P4C_BINARY_DIR_ORIG ${P4C_BINARY_DIR})
  set(P4C_BINARY_DIR ${BFASM_BINARY_DIR})

  set (JBAY_INCLUDE_PATTERNS "target:.*Tofino2")
  #set (JBAY_EXCLUDE_PATTERNS )
  #set (JBAY_EXCLUDE_FILES )

  set (BFA_FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/stf/*.bfa")
  p4c_find_tests("${BFA_FILES}" JBAY_TESTS_PRE1 INCLUDE "${JBAY_INCLUDE_PATTERNS}" EXCLUDE "${JBAY_EXCLUDE_PATTERNS}")
  bfn_find_tests("${JBAY_TESTS_PRE1}" JBAY_TESTS EXCLUDE "${JBAY_EXCLUDE_FILES}")

  bfas_add_tests("tofino2_asm" "jbay" "bfas_jbay" "base" "${JBAY_TESTS}")

  # Restore the P4C source/binary dirs
  set(P4C_SOURCE_DIR ${P4C_SOURCE_DIR_ORIG})
  set(P4C_BINARY_DIR ${P4C_BINARY_DIR_ORIG})
endif()

