# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

# Each scenario owns its build directories and cache, including across test runs.
file(REMOVE_RECURSE "${TEST_BINARY_DIR}")
file(MAKE_DIRECTORY "${TEST_BINARY_DIR}")
file(COPY "${FIXTURE_ROOT}/package.tar" DESTINATION "${TEST_BINARY_DIR}")
file(SHA256 "${TEST_BINARY_DIR}/package.tar" archive_hash)

# A developer's environment must not change the selection rules under test.
foreach(option CPM_DOWNLOAD_ALL CPM_DOWNLOAD_P4CFixture CPM_USE_LOCAL_PACKAGES
               CPM_LOCAL_PACKAGES_ONLY)
  unset(ENV{${option}})
endforeach()

function(check_configure build_name)
  execute_process(
    COMMAND ${CMAKE_COMMAND}
      -S "${CMAKE_CURRENT_LIST_DIR}/dependency"
      -B "${TEST_BINARY_DIR}/${build_name}"
      -DSCENARIO=${SCENARIO}
      -DCPM_IMPLEMENTATION=${CPM_IMPLEMENTATION}
      -DCPM_SOURCE_CACHE=${TEST_BINARY_DIR}/cache
      -DFIXTURE_ROOT=${FIXTURE_ROOT}
      -DFIXTURE_ARCHIVE=${TEST_BINARY_DIR}/package.tar
      -DFIXTURE_HASH=${archive_hash}
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors)

  if(SCENARIO STREQUAL "missing-installed")
    if(result EQUAL 0 OR NOT errors MATCHES "P4CFixture not found via find_package")
      message(FATAL_ERROR "Missing installed packages must fail configuration:\n${output}\n${errors}")
    endif()
  elseif(NOT result EQUAL 0)
    message(FATAL_ERROR "${SCENARIO} configuration failed:\n${output}\n${errors}")
  endif()

  if(SCENARIO STREQUAL "parent-older")
    if(NOT errors MATCHES "Requires a newer version of P4CFixture")
      message(FATAL_ERROR "Reusing an older parent package must produce a version warning")
    endif()
  elseif(SCENARIO STREQUAL "parent-newer")
    if(errors MATCHES "Requires a newer version of P4CFixture")
      message(FATAL_ERROR "Reusing a newer parent package must not produce a version warning")
    endif()
  endif()
endfunction()

check_configure(first)
if(SCENARIO STREQUAL "source-cache")
  # The second build must reuse the cached sources after the archive is removed.
  file(REMOVE "${TEST_BINARY_DIR}/package.tar")
  check_configure(second)
  file(READ "${TEST_BINARY_DIR}/first/selected-source.txt" first_source)
  file(READ "${TEST_BINARY_DIR}/second/selected-source.txt" second_source)
  if(NOT first_source STREQUAL second_source)
    message(FATAL_ERROR "Separate builds did not reuse the same cached sources")
  endif()
endif()
