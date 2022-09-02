# Copyright 2013-present Barefoot Networks, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.h.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/version.h" @ONLY)

set (GRAPHS_SRCS
  graphs.cpp
  controls.cpp
  parsers.cpp
  graph_visitor.cpp
  p4c-graphs.cpp
  )

set (GRAPHS_HDRS
  graphs.h
  controls.h
  parsers.h
  graph_visitor.h
  )

add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "${GRAPHS_SRCS};${GRAPHS_HDRS}")

build_unified(GRAPHS_SRCS ALL)
add_executable(p4c-graphs ${GRAPHS_SRCS} ${EXTENSION_P4_14_CONV_SOURCES})
target_link_libraries (p4c-graphs ${P4C_LIBRARIES} ${P4C_LIB_DEPS})
add_dependencies(p4c-graphs genIR frontend)

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
