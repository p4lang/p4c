# ccache integration for p4c builds.

include_guard(GLOBAL)

option(ENABLE_CCACHE "Enable ccache compiler launcher when available" ON)

if(NOT ENABLE_CCACHE)
  return()
endif()

find_program(CCACHE_PROGRAM ccache)
if(NOT CCACHE_PROGRAM)
  return()
endif()

message(STATUS "Enabling ccache: ${CCACHE_PROGRAM}")

foreach(LANG C CXX)
  if(NOT CMAKE_${LANG}_COMPILER_LAUNCHER)
    set(CMAKE_${LANG}_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  endif()
endforeach()
