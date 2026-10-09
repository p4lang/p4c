# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.16.3)
project(P4CDependencyTests NONE)
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include(CPM)

# Use small local packages so that dependency selection needs neither a compiler
# nor network access after the CPM bootstrap has been downloaded.
set(fixture_root "${CMAKE_CURRENT_BINARY_DIR}/fixtures")
function(make_source_fixture directory version)
  file(WRITE "${fixture_root}/${directory}/CMakeLists.txt"
    "add_library(P4CFixture INTERFACE)\n"
    "set_property(TARGET P4CFixture PROPERTY fixture_origin source)\n"
    "set_property(TARGET P4CFixture PROPERTY fixture_version ${version})\n"
    "set_property(TARGET P4CFixture PROPERTY fixture_shared \"\${BUILD_SHARED_LIBS}\")\n")
endfunction()
make_source_fixture(pinned 1.0)
make_source_fixture(checkout 2.0)
make_source_fixture(older 0.5)

# A local archive exercises CPM's normal download and cache paths without HTTP.
execute_process(
  COMMAND ${CMAKE_COMMAND} -E tar cf "${fixture_root}/package.tar" --format=gnutar CMakeLists.txt
  WORKING_DIRECTORY "${fixture_root}/pinned"
  RESULT_VARIABLE archive_result)
if(NOT archive_result EQUAL 0)
  message(FATAL_ERROR "Could not create the dependency fixture archive")
endif()

set(package_dir "${fixture_root}/prefix/lib/cmake/P4CFixture")
file(WRITE "${package_dir}/P4CFixtureConfig.cmake"
  "add_library(P4CFixture INTERFACE IMPORTED GLOBAL)\n"
  "set_property(TARGET P4CFixture PROPERTY fixture_origin installed)\n"
  "set_property(TARGET P4CFixture PROPERTY fixture_version 1.0)\n")
include(CMakePackageConfigHelpers)
write_basic_package_version_file("${package_dir}/P4CFixtureConfigVersion.cmake"
  VERSION 1.0 COMPATIBILITY SameMajorVersion ARCH_INDEPENDENT)

enable_testing()
foreach(scenario
    default-source package-source installed-prefix installed-dir source-fallback
    prefer-installed require-installed missing-installed source-override
    parent-older parent-newer source-cache)
  add_test(NAME cpm-${scenario}
    COMMAND ${CMAKE_COMMAND}
      -DSCENARIO=${scenario}
      -DCPM_IMPLEMENTATION=${CPM_FILE}
      -DFIXTURE_ROOT=${fixture_root}
      -DTEST_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/${scenario}
      -P ${CMAKE_CURRENT_LIST_DIR}/configure.cmake)
endforeach()
