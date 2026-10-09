# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

# Configure-only integration checks. No compiler or dependency downloads are
# needed once the CPM bootstrap is present in CPM_SOURCE_CACHE.
cmake_minimum_required(VERSION 3.16.3)
project(P4CDependencyTests NONE)
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/.." "${CMAKE_CURRENT_BINARY_DIR}/modules")
include(P4CDependencies)
include(CPM)

function(fixture name)
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${name}/CMakeLists.txt"
    "add_library(${name} INTERFACE)\n"
    "set_property(TARGET ${name} PROPERTY fixture_origin source)\n"
    "set_property(TARGET ${name} PROPERTY fixture_shared \"\${BUILD_SHARED_LIBS}\")\n")
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/modules/Find${name}.cmake"
    "add_library(${name} INTERFACE IMPORTED)\n"
    "set_property(TARGET ${name} PROPERTY fixture_origin installed)\n"
    "set(${name}_FOUND TRUE)\n")
endfunction()

# A parent's source package may have been added in a sibling directory, so
# only CPM's registry (not inherited *_SOURCE_DIR variables) records it here.
fixture(SharedFixture)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/owner/CMakeLists.txt"
  "CPMAddPackage(NAME SharedFixture SOURCE_DIR \"${CMAKE_CURRENT_BINARY_DIR}/SharedFixture\")\n")
add_subdirectory("${CMAKE_CURRENT_BINARY_DIR}/owner" owner-build)
set(CPM_DOWNLOAD_ALL OFF)
p4c_find_package(NAME SharedFixture FIND_PACKAGE_ARGUMENTS MODULE)
get_target_property(origin SharedFixture fixture_origin)
if(NOT origin STREQUAL "source" OR SharedFixture_ADDED OR NOT SharedFixture_SOURCE_DIR)
  message(FATAL_ERROR "A parent's shared dependency was not reused")
endif()

# An explicit checkout takes priority even when installed packages are preferred.
fixture(OverrideFixture)
set(CPM_OverrideFixture_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/OverrideFixture")
set(CPM_DOWNLOAD_OverrideFixture OFF)
set(BUILD_SHARED_LIBS ON)
p4c_find_package(NAME OverrideFixture FIND_PACKAGE_ARGUMENTS MODULE OPTIONS "BUILD_SHARED_LIBS OFF")
get_target_property(origin OverrideFixture fixture_origin)
get_target_property(shared OverrideFixture fixture_shared)
if(NOT origin STREQUAL "source" OR NOT OverrideFixture_ADDED OR shared OR NOT BUILD_SHARED_LIBS)
  message(FATAL_ERROR "Source overrides or scoped dependency options were not respected")
endif()

# Installed dependencies must not be mistaken for newly created source targets.
fixture(InstalledFixture)
set(InstalledFixture_ADDED YES)
set(InstalledFixture_SOURCE_DIR stale)
p4c_find_package(NAME InstalledFixture FIND_PACKAGE_ARGUMENTS MODULE)
get_target_property(origin InstalledFixture fixture_origin)
if(NOT origin STREQUAL "installed" OR InstalledFixture_ADDED OR InstalledFixture_SOURCE_DIR)
  message(FATAL_ERROR "Installed dependency result contains stale source state")
endif()

# Re-including P4C's bootstrap must leave the parent's CPM instance alone.
set(parent_cpm "${CPM_FILE}")
include("${CMAKE_CURRENT_LIST_DIR}/../CPM.cmake")
if(NOT CPM_FILE STREQUAL parent_cpm OR CPM_DOWNLOAD_ALL)
  message(FATAL_ERROR "The parent's CPM instance or settings changed")
endif()
message(STATUS "P4C dependency integration checks passed")

enable_testing()
foreach(scenario installed config source missing)
  add_test(NAME p4runtime-${scenario}
    COMMAND ${CMAKE_COMMAND}
      -DSCENARIO=${scenario}
      -DCPM_IMPLEMENTATION=${CPM_FILE}
      -DTEST_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/p4runtime-${scenario}
      -P ${CMAKE_CURRENT_LIST_DIR}/p4runtime.cmake)
endforeach()
