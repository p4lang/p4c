# Common configuration across all P4Tools projects.

# Use ccache if available.
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  message(STATUS "Enabling ccache")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

# Use C++11.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use libgc.
find_package(LibGc 7.2.0 REQUIRED)

# Helper for defining a p4tools executable target.
function(add_p4tools_executable target source)
  add_executable(${target} ${source} ${ARGN})
  target_include_directories(
    ${target}
    PUBLIC "${CMAKE_SOURCE_DIR}"
    PUBLIC "${CMAKE_BINARY_DIR}"
  )
  install(TARGETS ${target} RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})
endfunction(add_p4tools_executable)

# Helper for defining a p4tools library target.
function(add_p4tools_library target)
  add_library(${target} ${ARGN})
  target_include_directories(
    ${target}
    PUBLIC "${CMAKE_SOURCE_DIR}"
    PUBLIC "${CMAKE_BINARY_DIR}"
  )
endfunction(add_p4tools_library)
