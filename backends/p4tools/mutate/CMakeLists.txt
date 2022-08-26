cmake_minimum_required(VERSION 3.0.2 FATAL_ERROR)

include(common)

project(mutate)


# Source files for mutate.
set(
  MUTATE_SOURCES
  options.cpp
  mutate.cpp
)

add_p4tools_library(mutate ${MUTATE_SOURCES})

set(
  MUTATE_LIBS
  p4testgen-common
  ${P4C_LIBRARIES}
  ${P4C_LIB_DEPS}
  ${CMAKE_THREAD_LIBS_INIT}
)

target_link_libraries(
  mutate
  ${MUTATE_LIBS}
)
