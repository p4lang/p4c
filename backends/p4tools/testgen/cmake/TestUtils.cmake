# generate all the tests specified in the tests list
# Arguments:
#   - template_file The path to the extension/target file that implements
#     the logic of the target test. "p4tools_add_test_list" includes this file and then
#     executes the logic defined "p4tools_add_test_with_args".
#   - tag is a label for the set of test suite where this test belongs
#     (for example, p4ctest)
#   - driver is the script that is used to run the test and compare the results
#   - tests is a list of test names
#   - xfail is a set of tests that are expected to fail
#   - target is the target to test against
#   - arch is the p4 architecture
#   - std is the p4 standard
#   - enable_runner is the flag to compile the p4 and run the runner
#
# The macro generates the test files in a directory prefixed by tag.
#
macro(p4tools_add_test_list template_file tag driver tests xfail target arch std enable_runner)
  set(__xfail_list "${xfail}")
  set(__test_list "${tests}")
  set(__testCounter 0)
  set(__xfailCounter 0)
  list(LENGTH __test_list __nTests)
  include(${template_file})
  foreach(t ${__test_list})
    list(FIND __xfail_list ${t} __xfail_test)
    get_filename_component(p4name ${t} NAME)
    if(__xfail_test GREATER -1)
      p4tools_add_test_with_args(
        ${tag} ${driver} TRUE ${p4name} ${t}
        ${target} ${arch} ${std}
        ${enable_runner} "${ARGN}" ""
      )
      math(EXPR __xfailCounter "${__xfailCounter} + 1")
    else()
      p4tools_add_test_with_args(
        ${tag} ${driver} FALSE ${p4name} ${t}
        ${target} ${arch} ${std}
        ${enable_runner} "${ARGN}" ""
      )
    endif() # __xfail_test
  endforeach() # tests
  math(EXPR __testCounter "${__testCounter} + ${__nTests}")
  # add the tag to the list
  set(TEST_TAGS ${TEST_TAGS} ${tag} CACHE INTERNAL "test tags")
  message(STATUS "Added ${__testCounter} tests to '${tag}' (${__xfailCounter} xfails)")
endmacro(p4tools_add_test_list)

# generate all the tests specified in the testsuites: builds a list of tests
# from the testsuite patterns by calling p4c_find_test_names then pass the list
# to p4tools_add_test_list
# Arguments:
#   - template_file The path to the extension/target file that implements
#     the logic of the target test. "p4tools_add_tests" includes this file and then
#     executes the logic defined "p4tools_add_test_with_args".
#   - tag is a label for the set of test suite where this test belongs
#     (for example, p4ctest)
#   - driver is the script that is used to run the tests and compare the results
#   - testsuites is a list of directory patterns, e.g.: testdata/p4_16_samples/*.p4
#   - xfails is a set of tests that are expected to fail
#   - target is the target to test against
#   - arch is the p4 architecture
#   - std is the p4 standard
#   - enable_runner is the flag to compile the p4 and run the runner
#   -- if true, run the runner
#   -- if false, run the PTF test
#
# The macro generates the test files in a directory prefixed by tag.
#
macro(p4tools_add_tests template_file tag driver testsuites xfails target arch std enable_runner)
  set(__tests "")
  set(__xfails "")
  p4c_find_test_names("${testsuites}" __tests)
  p4c_find_test_names("${xfails}" __xfails)
  p4tools_add_test_list(
    ${template_file}
    ${tag} ${driver} "${__tests}" "${__xfails}"
    ${target} ${arch} ${std} ${enable_runner} "${ARGN}"
  )
endmacro(p4tools_add_tests)

# Used to add a label to the test like xfail
macro(p4tools_add_test_label tag newLabel testname)
  set(__testname ${tag}/${testname})
  get_property(__labels TEST ${__testname} PROPERTY LABELS)
  set_tests_properties(${__testname} PROPERTIES LABELS "${__labels};${newLabel}")
endmacro(p4tools_add_test_label)

# if we have a reason for failure, then use that regular expression to
# make the test succeed. If that changes, we know the test moved to a
# different failure. Also turn off automatic ignoring of failures (WILL_FAIL).
macro(p4tools_add_xfail_reason tag reason)
  set(__tests "${ARGN}")
  string(TOUPPER ${tag} __upperTag)
  foreach(test IN LISTS __tests)
    list(FIND ${__upperTag}_MUST_PASS_TESTS ${test} __isMustPass)
    if(${__isMustPass} EQUAL -1) # not a mandatory pass test
      get_filename_component(p4name ${test} NAME)
      p4c_test_set_name(__testname ${tag} ${p4name})
      if("${reason}" STREQUAL "")
        set_tests_properties(${__testname} PROPERTIES WILL_FAIL TRUE)
      else()
        set_tests_properties(
          ${__testname} PROPERTIES
          PASS_REGULAR_EXPRESSION ${reason}
          WILL_FAIL 0
        )
      endif()
      p4tools_add_test_label(${tag} "XFAIL" ${test})
    else()
      message(WARNING "${test} can not be listed as an xfail. It must always pass!")
    endif()
  endforeach()
endmacro(p4tools_add_xfail_reason)

# find all the tests, except the ones matching the names in patterns
function(p4tools_find_tests input_files test_list exclude patterns)
  set(inputList "")
  foreach(__t ${input_files})
    file(GLOB __inputTest ${__t})
    list(APPEND inputList ${__inputTest})
  endforeach()
  list(REMOVE_DUPLICATES inputList)

  # remove the tests that match one of the regexes in patterns
  foreach(f IN LISTS inputList)
    foreach(pattern IN LISTS patterns)
      string(REGEX MATCH ${pattern} __found ${f})
      if(NOT ${__found})
        list(REMOVE_ITEM inputList ${f})
        break()
      endif()
    endforeach()
  endforeach()
  set(${test_list} ${inputList} PARENT_SCOPE)
endfunction()
