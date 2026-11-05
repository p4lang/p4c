# SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
# Copyright 2013-present Barefoot Networks, Inc.
#
# SPDX-License-Identifier: Apache-2.0

if (P4C_USE_PREINSTALLED_MULTIPRECISION)
  set(BOOST_MIN_VERSION 1.58)
else ()
  # Older Boost.Graph headers conflict with standalone Multiprecision.
  set(BOOST_MIN_VERSION 1.83)
endif ()
find_package(Boost ${BOOST_MIN_VERSION} QUIET)
if (NOT Boost_FOUND)
  if (NOT P4C_USE_PREINSTALLED_MULTIPRECISION)
    message(FATAL_ERROR
      "The graphs backend requires Boost ${BOOST_MIN_VERSION} or newer headers. "
      "Older Boost.Graph headers conflict with fetched standalone Multiprecision. "
      "Install a compatible Boost version (set BOOST_ROOT for a custom installation), "
      "or configure with -DP4C_USE_PREINSTALLED_MULTIPRECISION=ON to use matching "
      "installed Multiprecision and Boost headers. To disable this backend, set "
      "-DENABLE_P4C_GRAPHS=OFF.")
  else ()
    message(FATAL_ERROR
      "The graphs backend requires Boost ${BOOST_MIN_VERSION} or newer headers. "
      "Install them or set BOOST_ROOT to a compatible installation. "
      "To disable this backend, set -DENABLE_P4C_GRAPHS=OFF.")
  endif ()
endif ()

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
# Graphviz output uses only the header-only part of Boost.Graph.
target_link_libraries(p4cgraphs ir-generated frontend Boost::headers)

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
