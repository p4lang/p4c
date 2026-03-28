# Common configuration across all P4Tools projects.

# Helper for defining a p4tools executable target.
function(add_p4tools_executable target source)
  add_executable(${target} ${source} ${ARGN})
  install(TARGETS ${target} RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})
endfunction(add_p4tools_executable)
