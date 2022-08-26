cmake_minimum_required(VERSION 3.0.2 FATAL_ERROR)

include(common)

project(smith)


# Source files for smith.
set(
  SMITH_SOURCES
  options.cpp
  smith.cpp

)


add_p4tools_library(smith ${SMITH_SOURCES})


set(
  SMITH_LIBS
  p4testgen-common
  ${P4C_LIBRARIES}
  ${P4C_LIB_DEPS}
  ${CMAKE_THREAD_LIBS_INIT}
)

target_link_libraries(
  smith
  ${SMITH_LIBS}
)
