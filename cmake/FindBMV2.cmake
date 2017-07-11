
set(BMV2_SEARCH_PATHS
  ${CMAKE_INSTALL_PREFIX}/bin
  ${P4C_SOURCE_DIR}/../behavioral-model/build/targets/simple_switch
  ${P4C_SOURCE_DIR}/../../behavioral-model/build/targets/simple_switch)

# check for simple_switch
find_program (SIMPLE_SWITCH_CLI simple_switch_CLI
  PATHS ${BMV2_SEARCH_PATHS} )
if (SIMPLE_SWITCH_CLI)
  find_program (SIMPLE_SWITCH simple_switch
    PATHS ${BMV2_SEARCH_PATHS} )
  if (SIMPLE_SWITCH)
    set (HAVE_SIMPLE_SWITCH 1)
    find_python_module (scapy REQUIRED)
    find_python_module (ipaddr REQUIRED)
  endif(SIMPLE_SWITCH)
endif(SIMPLE_SWITCH_CLI)

mark_as_advanced(SIMPLE_SWITCH SIMPLE_SWITCH_CLI)

find_package_handle_standard_args ("BMv2 programs"
  "Program 'simple_switch_CLI' (https://github.com/p4lang/behavioral-model.git) not found;\nSearched ${BMV2_SEARCH_PATHS}.\nWill not run BMv2 tests."
  SIMPLE_SWITCH SIMPLE_SWITCH_CLI)
