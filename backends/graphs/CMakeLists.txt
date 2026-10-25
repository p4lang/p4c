# SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
# Copyright 2013-present Barefoot Networks, Inc.
#
# SPDX-License-Identifier: Apache-2.0

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.h.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/version.h" @ONLY)

set (GRAPHS_SRCS
  graphs.cpp
  controls.cpp
  parsers.cpp
  graph_visitor.cpp
)

set (GRAPHS_HDRS
  graphs.h
  controls.h
  parsers.h
  graph_visitor.h
)

add_library(p4cgraphs STATIC ${GRAPHS_SRCS} ${EXTENSION_P4_14_CONV_SOURCES})
target_link_libraries(p4cgraphs ir-generated frontend)

add_executable(p4c-graphs p4c-graphs.cpp)
target_link_libraries (p4c-graphs p4cgraphs ir-generated frontend ${P4C_LIBRARIES} ${P4C_LIB_DEPS})

install (TARGETS p4c-graphs
  RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})

file(RELATIVE_PATH
  CURRENT_BINARY_DIR_PATH_REL
  ${P4C_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

# This link is not required but it is convenient to have access to this
# backend in the top-level build directory, along with all the other backends.
add_custom_target(linkgraphs
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CURRENT_BINARY_DIR_PATH_REL}/p4c-graphs ${P4C_BINARY_DIR}/p4c-graphs
  DEPENDS update_includes
  )
add_dependencies(p4c_driver linkgraphs)

# add tests -- all P4 files in p4_{14,16}_samples that contain 'graph' in name
# are considered tests for the graph backend
set(GRAPH_TEST_DRIVER ${P4C_SOURCE_DIR}/backends/graphs/run-graph-sample.sh)
set(GRAPH_TEST_SUITES
  "${P4C_SOURCE_DIR}/testdata/p4_14_samples/*graph*.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/*graph*.p4")
set(GRAPH_TEST_XFAILS "")
p4c_add_tests("graph" ${GRAPH_TEST_DRIVER} "${GRAPH_TEST_SUITES}" "${GRAPH_TEST_XFAILS}")
