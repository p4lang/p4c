# Common configuration across all P4Tools projects.

# Use ccache if available.
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  message(STATUS "Enabling ccache")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

# Helper for defining a p4tools executable target.
function(add_p4tools_executable target source)
  add_executable(${target} ${source} ${ARGN})
  install(TARGETS ${target} RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})
endfunction(add_p4tools_executable)

