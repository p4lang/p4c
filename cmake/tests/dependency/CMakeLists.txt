# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.16.3)
project(P4CDependencyScenario NONE)
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../..")

# Start with installed packages preferred when testing the package-specific
# source setting and reuse of a parent's existing dependency.
if(SCENARIO STREQUAL "package-source" OR SCENARIO MATCHES "^parent-")
  set(CPM_DOWNLOAD_ALL OFF)
endif()
include(P4CDependencies)
include("${CPM_IMPLEMENTATION}")

set(expected_origin source)
set(expected_version 1.0)
set(expected_shared OFF)
set(BUILD_SHARED_LIBS ON)

# Offer an installed package except where the scenario requires its absence.
if(NOT SCENARIO STREQUAL "source-fallback" AND NOT SCENARIO STREQUAL "missing-installed")
  if(SCENARIO STREQUAL "installed-dir")
    set(P4CFixture_DIR "${FIXTURE_ROOT}/prefix/lib/cmake/P4CFixture")
  else()
    list(PREPEND CMAKE_PREFIX_PATH "${FIXTURE_ROOT}/prefix")
  endif()
endif()

if(SCENARIO STREQUAL "package-source")
  set(CPM_DOWNLOAD_P4CFixture ON)
elseif(SCENARIO MATCHES "^installed-" OR SCENARIO STREQUAL "source-fallback")
  set(CPM_DOWNLOAD_P4CFixture OFF)
  if(NOT SCENARIO STREQUAL "source-fallback")
    set(expected_origin installed)
  endif()
elseif(SCENARIO STREQUAL "prefer-installed")
  set(CPM_USE_LOCAL_PACKAGES ON)
  set(expected_origin installed)
elseif(SCENARIO STREQUAL "require-installed" OR SCENARIO STREQUAL "missing-installed")
  set(CPM_LOCAL_PACKAGES_ONLY ON)
  set(expected_origin installed)
elseif(SCENARIO STREQUAL "source-override")
  # An explicit checkout wins even when installed packages are required.
  set(CPM_LOCAL_PACKAGES_ONLY ON)
  set(CPM_DOWNLOAD_P4CFixture OFF)
  set(CPM_P4CFixture_SOURCE "${FIXTURE_ROOT}/checkout")
  set(expected_version 2.0)
elseif(SCENARIO MATCHES "^parent-")
  if(SCENARIO STREQUAL "parent-older")
    set(parent_source "${FIXTURE_ROOT}/older")
    set(expected_version 0.5)
  else()
    set(parent_source "${FIXTURE_ROOT}/checkout")
    set(expected_version 2.0)
  endif()
  # Only CPM's registry records packages added in a sibling directory.
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/owner/CMakeLists.txt"
    "CPMAddPackage(NAME P4CFixture VERSION ${expected_version}\n"
    "  SOURCE_DIR \"${parent_source}\" OPTIONS \"BUILD_SHARED_LIBS ON\")\n")
  add_subdirectory("${CMAKE_CURRENT_BINARY_DIR}/owner" owner-build)
  set(expected_shared ON)
  # Neither installed discovery nor a checkout override may replace the parent.
  set(CPM_P4CFixture_SOURCE "${FIXTURE_ROOT}/pinned")

  set(parent_cpm "${CPM_FILE}")
  set(parent_cache "${CPM_SOURCE_CACHE}")
  include("${CMAKE_CURRENT_LIST_DIR}/../../CPM.cmake")
  if(NOT CPM_FILE STREQUAL parent_cpm OR NOT CPM_SOURCE_CACHE STREQUAL parent_cache
     OR CPM_DOWNLOAD_ALL)
    message(FATAL_ERROR "P4C changed the parent's CPM instance or settings")
  endif()
endif()

# Installed discovery must clear source state left by earlier calls.
set(P4CFixture_ADDED YES)
set(P4CFixture_SOURCE_DIR stale)
set(P4CFixture_BINARY_DIR stale)
p4c_find_package(
  NAME P4CFixture
  VERSION 1.0
  URL "${FIXTURE_ARCHIVE}"
  URL_HASH SHA256=${FIXTURE_HASH}
  FIND_PACKAGE_ARGUMENTS CONFIG
  OPTIONS "BUILD_SHARED_LIBS OFF")

get_target_property(origin P4CFixture fixture_origin)
get_target_property(version P4CFixture fixture_version)
if(NOT origin STREQUAL expected_origin OR NOT version VERSION_EQUAL expected_version)
  message(FATAL_ERROR
    "Expected ${expected_origin} version ${expected_version}, selected ${origin} version ${version}")
endif()
if(NOT BUILD_SHARED_LIBS)
  message(FATAL_ERROR "Dependency options changed the parent's BUILD_SHARED_LIBS setting")
endif()

if(expected_origin STREQUAL "installed")
  if(P4CFixture_ADDED OR P4CFixture_SOURCE_DIR OR P4CFixture_BINARY_DIR)
    message(FATAL_ERROR "Installed dependency contains stale source state")
  endif()
else()
  get_target_property(shared P4CFixture fixture_shared)
  if(NOT shared STREQUAL expected_shared)
    message(FATAL_ERROR "Expected BUILD_SHARED_LIBS=${expected_shared} for the dependency, got ${shared}")
  endif()
  if(SCENARIO MATCHES "^parent-")
    if(P4CFixture_ADDED OR NOT P4CFixture_SOURCE_DIR STREQUAL parent_source)
      message(FATAL_ERROR "The parent's dependency was not reused")
    endif()
  elseif(NOT P4CFixture_ADDED OR NOT IS_DIRECTORY "${P4CFixture_SOURCE_DIR}")
    message(FATAL_ERROR "The selected source package was not added")
  endif()
endif()

if(SCENARIO STREQUAL "source-cache")
  string(FIND "${P4CFixture_SOURCE_DIR}" "${CPM_SOURCE_CACHE}/" source_prefix)
  string(FIND "${P4CFixture_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/" binary_prefix)
  if(NOT source_prefix EQUAL 0 OR NOT binary_prefix EQUAL 0)
    message(FATAL_ERROR "Sources must use the shared cache and binaries must use the build directory")
  endif()
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/selected-source.txt" "${P4CFixture_SOURCE_DIR}")
endif()
