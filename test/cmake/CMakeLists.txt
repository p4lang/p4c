# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

add_test(NAME cmake/test-generation
  COMMAND ${CMAKE_COMMAND} -E env
    "CMAKE_COMMAND=${CMAKE_COMMAND}" "CTEST_COMMAND=${CMAKE_CTEST_COMMAND}"
    ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_test_generation.py)
set_tests_properties(cmake/test-generation PROPERTIES LABELS "cmake")

add_test(NAME cmake/lint-files
  COMMAND ${CMAKE_COMMAND} -E env "CMAKE_COMMAND=${CMAKE_COMMAND}"
    ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_lint_files.py)
set_tests_properties(cmake/lint-files PROPERTIES LABELS "cmake")
