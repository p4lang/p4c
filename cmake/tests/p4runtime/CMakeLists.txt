# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.16.3)
project(P4RuntimeDiscoveryTest NONE)
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../..")
include(P4CDependencies)
include("${CPM_IMPLEMENTATION}")
include(P4Runtime)

set(CPM_LOCAL_PACKAGES_ONLY ON)
set(p4runtime_ROOT "${CMAKE_CURRENT_BINARY_DIR}/prefix")
set(installed_schemas "${p4runtime_ROOT}/share/p4runtime/proto")
if(SCENARIO STREQUAL "config")
  # A package may install its schemas somewhere outside our fallback paths.
  set(installed_schemas "${p4runtime_ROOT}/custom/schemas")
  file(WRITE "${p4runtime_ROOT}/share/cmake/p4runtime/p4runtimeConfig.cmake"
    "set(p4runtime_PROTO_DIR \"${installed_schemas}\")\n")
  file(WRITE "${p4runtime_ROOT}/share/cmake/p4runtime/p4runtimeConfigVersion.cmake"
    "set(PACKAGE_VERSION 1.6.0)\nset(PACKAGE_VERSION_COMPATIBLE TRUE)\n")
endif()
set(source_schemas "${CMAKE_CURRENT_BINARY_DIR}/checkout/proto")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/checkout/CMakeLists.txt"
  "cmake_minimum_required(VERSION 3.16)\n"
  "project(p4runtime NONE)\n"
  "add_library(p4runtime_schemas INTERFACE)\n"
  "add_library(p4runtime::schemas ALIAS p4runtime_schemas)\n")
foreach(schema p4/config/v1/p4info.proto p4/config/v1/p4types.proto
               p4/v1/p4runtime.proto p4/v1/p4data.proto google/rpc/status.proto)
  file(WRITE "${source_schemas}/${schema}" "syntax = \"proto3\";\n")
  if(NOT SCENARIO STREQUAL "missing" OR NOT schema STREQUAL "p4/config/v1/p4types.proto")
    file(WRITE "${installed_schemas}/${schema}" "syntax = \"proto3\";\n")
  endif()
endforeach()

if(SCENARIO STREQUAL "source")
  set(CPM_p4runtime_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/checkout")
  set(expected_schemas "${source_schemas}")
else()
  set(expected_schemas "${installed_schemas}")
endif()
p4c_obtain_p4runtime()
if(SCENARIO STREQUAL "source" AND NOT TARGET p4runtime::schemas)
  message(FATAL_ERROR "The source package's CMake configuration was not loaded")
endif()
if(NOT P4RUNTIME_STD_DIR STREQUAL expected_schemas)
  message(FATAL_ERROR "Expected ${expected_schemas}, selected ${P4RUNTIME_STD_DIR}")
endif()
get_property(cached_schemas CACHE P4RUNTIME_STD_DIR PROPERTY VALUE)
if(NOT cached_schemas STREQUAL expected_schemas)
  message(FATAL_ERROR "The schema cache entry is stale: ${cached_schemas}")
endif()
