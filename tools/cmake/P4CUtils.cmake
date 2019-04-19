include (CheckCCompilerFlag)
include (CheckCXXCompilerFlag)

# test and add a C++ compiler option if supported
MACRO (add_cxx_compiler_option option)
  string (REPLACE "+" "P" escaped_option1 ${option})
  string (REPLACE "-" "" escaped_option ${escaped_option1})
  set (_success "_HAVE_OPTION_${escaped_option}_")
  set (__required_flags_backup "${CMAKE_REQUIRED_FLAGS}")
  set (CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${${PROJECT_NAME}_CXX_FLAGS}")
  check_cxx_compiler_flag (${option} ${_success})
  set (CMAKE_REQUIRED_FLAGS ${__required_flags_backup})
  if (${_success})
    set (${PROJECT_NAME}_CXX_FLAGS "${${PROJECT_NAME}_CXX_FLAGS} ${option}")
  endif (${_success})
endmacro (add_cxx_compiler_option)

include (CheckCXXSourceCompiles)
# check if a compiler supports a certain feature by compiling the program
MACRO (check_compiler_feature option program var)
  set (__required_flags_backup "${CMAKE_REQUIRED_FLAGS}")
  set (CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${option}")

  check_cxx_source_compiles ("${program}" ${var})

  IF (${var} AND NOT "${option}" STREQUAL "")
    set (P4C_CXX_FLAGS "${P4C_CXX_FLAGS} ${option}")
  ENDIF ()
  if (${var})
    add_definitions (-D${var})
  endif ()
  set (CMAKE_REQUIRED_FLAGS ${__required_flags_backup})
endmacro (check_compiler_feature)

# add a required library
include (CheckLibraryExists)
# checks if a library exists and adds it to the list of p4c dependencies
# supports an additional argument: REQUIRED. If present, it will throw
# a fatal error if the library does not exist.
macro (p4c_add_library name symbol var)
  check_library_exists (${name} ${symbol} "" ${var})
  if (${var})
    set (P4C_LIB_DEPS "${P4C_LIB_DEPS};${name}")
  else()
    if("${ARGN}" STREQUAL "REQUIRED")
      MESSAGE (FATAL_ERROR "Can not find required library: ${name}")
    endif()
  endif()
endmacro(p4c_add_library)

# Add files with the appropriate path to the list of linted files
macro(add_cpplint_files dir filelist)
  foreach(__f ${filelist})
    string(REGEX MATCH "^/.*" abs_path "${__f}")
    if (NOT ${abs_path} EQUAL "")
      list (APPEND __cpplintFileList "${__f}")
    else()
      list (APPEND __cpplintFileList "${dir}/${__f}")
    endif()
  endforeach(__f)
  set (CPPLINT_FILES ${CPPLINT_FILES} ${__cpplintFileList} PARENT_SCOPE)
endmacro(add_cpplint_files)

macro(p4c_test_set_name name tag alias)
  set(${name} ${tag}/${alias})
endmacro(p4c_test_set_name)

# add a single test to the testsuite
# Arguments:
#   - tag is a label for the set of test suite where this test belongs
#     (for example, p4, p14_to_16, bmv2, ebpf
#   - driver is the script that is used to run the test and compare the results
#   - isXfail is boolean that specifies whether this test is expected to fail
#   - alias is a possibly different name for the test such that the
#     same p4 program can be used in different test configurations.
#     Must be unique across the test suite.
#   - p4test is the name of the p4 program to test (path relative to the p4c directory)
#   - args is a list of arguments to pass to the test
#
# It generates a ${p4name}.test file invoking ${driver} on the p4
# program with command line arguments ${args}
# Sets the timeout on tests at 300s (for the slow Travis machines)
#
macro(p4c_add_test_with_args tag driver isXfail alias p4test test_args cmake_args)
  set(__testfile "${P4C_BINARY_DIR}/${tag}/${p4test}.test")
  file (WRITE  ${__testfile} "#! /bin/bash\n")
  file (APPEND ${__testfile} "# Generated file, modify with care\n\n")
  file (APPEND ${__testfile} "cd ${P4C_BINARY_DIR}\n")
  file (APPEND ${__testfile} "${driver} ${P4C_SOURCE_DIR} ${test_args} \"$@\" ${P4C_SOURCE_DIR}/${p4test}")
  execute_process(COMMAND chmod +x ${__testfile})
  p4c_test_set_name(__testname ${tag} ${alias})
  separate_arguments(__args UNIX_COMMAND ${cmake_args})
  add_test (NAME ${__testname}
    COMMAND ${tag}/${p4test}.test ${__args}
    WORKING_DIRECTORY ${P4C_BINARY_DIR})
  if (NOT DEFINED ${tag}_timeout)
    set (${tag}_timeout 300)
  endif()
  if (${isXfail})
    set_tests_properties(${__testname} PROPERTIES LABELS "${tag};XFAIL" TIMEOUT ${${tag}_timeout} WILL_FAIL 1)
  else()
    set_tests_properties(${__testname} PROPERTIES LABELS ${tag} TIMEOUT ${${tag}_timeout})
  endif()
endmacro(p4c_add_test_with_args)

macro(p4c_add_test_label tag newLabel testname)
  set (__testname ${tag}/${testname})
  get_property(__labels TEST ${__testname} PROPERTY LABELS)
  set_tests_properties(${__testname} PROPERTIES LABELS "${__labels};${newLabel}")
endmacro(p4c_add_test_label)

# generate all the tests specified in the tests list
# Arguments:
#   - tag is a label for the set of tests, for example, p4, p14_to_16, bmv2, ebpf
#   - driver is the script that is used to run the tests and compare the results
#   - testsuite is a list of test names
#   - xfail is a set of tests that are expected to fail
#
# The macro generates the test files in a directory prefixed by tag.
#
macro(p4c_add_test_list tag driver tests xfail)
  set (__xfail_list "${xfail}")
  set (__test_list "${tests}")
  set (__testCounter 0)
  set (__xfailCounter 0)
  list (LENGTH __test_list __nTests)
  foreach(t ${__test_list})
    list (FIND __xfail_list ${t} __xfail_test)
    if(__xfail_test GREATER -1)
      p4c_add_test_with_args (${tag} ${driver} TRUE ${t} ${t} "${ARGN}" "")
      math (EXPR __xfailCounter "${__xfailCounter} + 1")
    else()
      p4c_add_test_with_args (${tag} ${driver} FALSE ${t} ${t} "${ARGN}" "")
    endif() # __xfail_test
  endforeach() # tests
  math (EXPR __testCounter "${__testCounter} + ${__nTests}")
  # add the tag to the list
  set (TEST_TAGS ${TEST_TAGS} ${tag} CACHE INTERNAL "test tags")
  MESSAGE(STATUS "Added ${__testCounter} tests to '${tag}' (${__xfailCounter} xfails)")
endmacro(p4c_add_test_list)

# generate a list of test name suffixes based on specified testsuites
function(p4c_find_test_names testsuites tests)
  set(__tests "")
  p4c_sanitize_path("${testsuites}" abs_paths)
  foreach(ts ${abs_paths})
    file (GLOB __testfiles RELATIVE ${P4C_SOURCE_DIR} ${ts})
    list (APPEND __tests ${__testfiles})
  endforeach()
  set(${tests} "${__tests}" PARENT_SCOPE)
endfunction()

# convert the paths from a list of input files to their absolute paths
# does not follow symlinks.
#   - files is a list of (relative) file paths
#   - abs_files is the return set of absolute file paths
function(p4c_sanitize_path files abs_files)
  foreach(file ${files})
    get_filename_component(__file "${file}"
                       ABSOLUTE BASE_DIR "${P4C_SOURCE_DIR}")
    list (APPEND __abs_files ${__file})
  endforeach()
  set(${abs_files} "${__abs_files}" PARENT_SCOPE)
endfunction()

# generate all the tests specified in the testsuites: builds a list of tests
# from the testsuite patterns by calling p4c_find_test_names then pass the list
# to p4c_add_test_list
# Arguments:
#   - tag is a label for the set of tests, for example, p4, p14_to_16, bmv2, ebpf
#   - driver is the script that is used to run the tests and compare the results
#   - testsuite is a list of directory patterns, e.g.: testdata/p4_16_samples/*.p4
#   - xfail is a set of tests that are expected to fail
#
# The macro generates the test files in a directory prefixed by tag.
#
macro(p4c_add_tests tag driver testsuites xfails)
  set(__tests "")
  set(__xfails "")
  p4c_find_test_names("${testsuites}" __tests)
  p4c_find_test_names("${xfails}" __xfails)
  p4c_add_test_list (${tag} ${driver} "${__tests}" "${__xfails}" "${ARGN}")
endmacro(p4c_add_tests)

# same as p4c_add_tests but adds --p4runtime flag when invoking test driver
# unless test is listed in p4rt_exclude
macro(p4c_add_tests_w_p4runtime tag driver testsuites xfails p4rt_exclude)
  set(__tests "")
  set(__xfails "")
  p4c_find_test_names("${testsuites}" __tests)
  p4c_find_test_names("${xfails}" __xfails)
  set(__tests_no_p4runtime "${p4rt_exclude}")
  set(__tests_p4runtime "${__tests}")
  list (REMOVE_ITEM __tests_p4runtime ${__tests_no_p4runtime})
  p4c_add_test_list (${tag} ${driver} "${__tests_no_p4runtime}" "${__xfails}" "${ARGN}")
  p4c_add_test_list (${tag} ${driver} "${__tests_p4runtime}" "${__xfails}" "--p4runtime ${ARGN}")
endmacro(p4c_add_tests_w_p4runtime)

# add rules to make check and recheck for a specific test suite
macro (p4c_add_make_check tag)
  if ( "${tag}" STREQUAL "all")
    set (__tests_regex "'.*'")
  else()
    # avoid escaping spaces
    set (__tests_regex "'${tag}/.*'")
  endif()
  get_filename_component (__logDir ${P4C_XFAIL_LOG} DIRECTORY)
  add_custom_target(check-${tag}
    # list the xfail tests
    COMMAND ${CMAKE_COMMAND} -E make_directory ${__logDir}
    COMMAND ${CMAKE_COMMAND} -E echo "XFAIL tests:" > ${P4C_XFAIL_LOG} &&
            ${CMAKE_CTEST_COMMAND} --show-only -L XFAIL  --tests-regex "${__tests_regex}" >> ${P4C_XFAIL_LOG}
    COMMAND ${CMAKE_CTEST_COMMAND} ${P4C_TEST_FLAGS} --tests-regex "${__tests_regex}"
    COMMENT "Running tests for tag ${tag}")
  add_custom_target(recheck-${tag}
    # list the xfail tests
    COMMAND ${CMAKE_COMMAND} -E make_directory ${__logDir}
    COMMAND ${CMAKE_COMMAND} -E echo "XFAIL tests:" > ${P4C_XFAIL_LOG} &&
            ${CMAKE_CTEST_COMMAND} --show-only -L XFAIL --tests-regex "${__tests_regex}" >> ${P4C_XFAIL_LOG}
    COMMAND ${CMAKE_CTEST_COMMAND} ${P4C_TEST_FLAGS} --tests-regex "${__tests_regex}" --rerun-failed
    COMMENT "Re-running failed tests for tag ${tag}")
endmacro(p4c_add_make_check)

# check for the number of cores on the machine and return them in ${var}
macro (p4c_get_nprocs var)
  if(APPLE)
    execute_process (COMMAND sysctl -n hw.logicalcpu
      OUTPUT_VARIABLE ${var}
      OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE rc)
  else()
    execute_process (COMMAND nproc
      OUTPUT_VARIABLE ${var}
      OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE rc)
  endif()
  # MESSAGE ("p4c_get_nprocs: ${rc} ${${var}}")
  if (NOT ${rc} EQUAL 0)
    set (${var} 4)
  endif()
endmacro (p4c_get_nprocs)

# poor's cmake replacement for $(shell grep -L xxx $(shell -l yyy))
# It takes as arguments:
#  - input_files: a pattern for files (full path)
#  - test_list: a variable to be set to the resulting list of tests matching
#  - pairs of:
#  - pattern selector: INCLUDE or EXCLUDE
#  - pattern list: a list of patterns that should be included or excluded
# Returns:
#  - the list of tests matching search_patterns and not exclude_patterns
function(p4c_find_tests input_files test_list incl_excl patterns)

  if (${ARGC} EQUAL 4)
    if (${ARGV2} STREQUAL INCLUDE)
      set(search_patterns "${ARGV3}")
    elseif (${ARGV2} STREQUAL EXCLUDE)
      set(exclude_patterns "${ARGV3}")
    else()
      MESSAGE (FATAL_ERROR "Invalid pattern selector ${ARGV2}")
    endif()
  elseif(${ARGC} EQUAL 6)
    if (${ARGV2} STREQUAL INCLUDE AND ${ARGV4} STREQUAL EXCLUDE)
      set(search_patterns "${ARGV3}")
      set(exclude_patterns "${ARGV5}")
    elseif (${ARGV4} STREQUAL INCLUDE AND ${ARGV2} STREQUAL EXCLUDE)
      set(search_patterns "${ARGV5}")
      set(exclude_patterns "${ARGV3}")
    else()
      MESSAGE (FATAL_ERROR "Invalid pattern selector combo: ${ARGV2}/${ARGV4}")
    endif()
  else()
    MESSAGE(FATAL_ERROR "Invalid number of arguments ${ARGC} for find_tests")
  endif()

  if (DEFINED search_patterns)
    list (LENGTH search_patterns __pLen)
  #   MESSAGE ("include pattern: ${search_patterns}, length ${__pLen}")
  else()
    # only exclude patterns
    set(__pLen 0)
  endif()
  # if (DEFINED exclude_patterns)
  #   MESSAGE ("exclude pattern: ${exclude_patterns}, length ${__pLen}")
  # endif()

  set (inputList "")
  foreach (l ${input_files})
    file (GLOB __inputList ${l})
    list (APPEND inputList ${__inputList})
  endforeach()
  list (REMOVE_DUPLICATES inputList)
  foreach (f IN LISTS inputList)
    set (__found 0)

    foreach(p ${search_patterns})
      file  (STRINGS ${f} __found_strings REGEX ${p})
      list (LENGTH __found_strings __found_len)
      if (__found_len GREATER 0)
        math (EXPR __found "${__found} + 1")
      endif()
    endforeach() # search_patterns

    foreach(p ${exclude_patterns})
      file  (STRINGS ${f} __found_strings REGEX ${p})
      list (LENGTH __found_strings __found_len)
      if (__found_len GREATER 0)
        set (__found -1)
      endif()
    endforeach() # exclude_patterns
    if (__found EQUAL __pLen)
      list (APPEND __p4tests ${f})
    endif()
  endforeach() # test files

  # return
  set(${test_list} ${__p4tests} PARENT_SCOPE)
endfunction(p4c_find_tests)
