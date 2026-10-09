# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

function(check_configure scenario)
  execute_process(
    COMMAND ${CMAKE_COMMAND} -S ${CMAKE_CURRENT_LIST_DIR}/p4runtime -B ${TEST_BINARY_DIR}
      -DSCENARIO=${scenario} -DCPM_IMPLEMENTATION=${CPM_IMPLEMENTATION}
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors)
  if(scenario STREQUAL "missing")
    if(result EQUAL 0 OR NOT errors MATCHES "Could NOT find p4runtime" OR
       output MATCHES "Adding package p4runtime")
      message(FATAL_ERROR "Incomplete installed schemas must fail without fetching:\n${output}\n${errors}")
    endif()
  elseif(NOT result EQUAL 0)
    message(FATAL_ERROR "P4Runtime ${scenario} configuration failed:\n${output}\n${errors}")
  endif()
endfunction()

check_configure(${SCENARIO})
if(SCENARIO STREQUAL "source")
  # Existing build directories must follow changes in dependency selection.
  check_configure(installed)
  check_configure(source)
endif()
