include (CheckCCompilerFlag)
include (CheckCXXCompilerFlag)

# test and add a C++ compiler option if supported
MACRO (add_cxx_compiler_option option)
  string (REPLACE "+" "P" escaped_option1 ${option})
  string (REPLACE "-" "" escaped_option ${escaped_option1})
  set (_success "_HAVE_OPTION_${escaped_option}_")
  set (__required_flags_backup "${CMAKE_REQUIRED_FLAGS}")
  set (CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${P4C_CXX_FLAGS}")
  check_cxx_compiler_flag (${option} ${_success})
  set (CMAKE_REQUIRED_FLAGS ${__required_flags_backup})
  if (${_success})
    set (P4C_CXX_FLAGS "${P4C_CXX_FLAGS} ${option}")
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
    list (APPEND CPPLINT_FILES "${dir}/${__f}")
  endforeach(__f)
endmacro(add_cpplint_files)

# generate all the tests specified in the testsuites
# Arguments:
#   - tag is a label for the set of tests, for example, p4, p14_to_16, bmv2, ebpf
#   - driver is the script that is used to run the tests and compare the results
#   - testsuite is a list of directory patterns, e.g.: testdata/p4_16_samples/*.p4
#   - xfail is a set of tests that are expected to fail
#
# The macro generates the test files in a directory prefixed by tag.
#
macro(p4c_add_tests tag driver testsuites xfail)
  set(__xfail_list "${xfail}")
  foreach(ts "${testsuites}")
    file (GLOB __testfiles RELATIVE ${P4C_SOURCE_DIR} ${ts})
    foreach(t ${__testfiles})
      set(__testfile "${P4C_BINARY_DIR}/${tag}/${t}.test")
      file (WRITE  ${__testfile} "#! /bin/bash\n")
      file (APPEND ${__testfile} "# Generated file, modify with care\n\n")
      file (APPEND ${__testfile} "cd ${P4C_BINARY_DIR}\n")
      file (APPEND ${__testfile} "${driver} ${P4C_SOURCE_DIR} $* ${P4C_SOURCE_DIR}/${t}")
      execute_process(COMMAND chmod +x ${__testfile})
      set(__testname ${tag}/${t})
      add_test (NAME ${__testname} COMMAND ${tag}/${t}.test WORKING_DIRECTORY ${P4C_BINARY_DIR})
      set_tests_properties(${__testname} PROPERTIES LABELS ${tag})
      list (LENGTH __xfail_list __xfail_length)
      if (${__xfail_length} GREATER 0)
        list (FIND __xfail_list ${t} __xfail_test)
        if(__xfail_test GREATER -1)
          set_tests_properties(${__testname} PROPERTIES WILL_FAIL 1 LABELS "XFAIL")
        endif() # __xfail_test
      endif() # __xfail_length
    endforeach()
  endforeach()
  # add the tag to the list
  set (TEST_TAGS ${TEST_TAGS} ${tag} CACHE INTERNAL "test tags")
endmacro(p4c_add_tests)
