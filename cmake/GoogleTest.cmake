# SPDX-FileCopyrightText: 2023 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

macro(p4c_obtain_googletest)
  p4c_find_package(
    NAME GTest
    VERSION 1.14.0
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG f8d7d77c06936315286eb55f8de22cd23c188571 # v1.14.0
    FIND_PACKAGE_ARGUMENTS CONFIG
    EXCLUDE_FROM_ALL YES
    SYSTEM YES
    OPTIONS "INSTALL_GTEST OFF" "gtest_disable_pthreads ON"
  )
  if(NOT TARGET gtest)
    # CMake < 3.18 cannot alias a non-global imported target.
    add_library(gtest INTERFACE)
    target_link_libraries(gtest INTERFACE GTest::gtest)
  endif()
  if(GTest_SOURCE_DIR)
    include_directories(BEFORE SYSTEM ${GTest_SOURCE_DIR}/googletest/include)
    include_directories(BEFORE SYSTEM ${GTest_SOURCE_DIR}/googlemock/include)
    if(GTest_ADDED)
      target_include_directories(gtest SYSTEM BEFORE PUBLIC
        ${GTest_SOURCE_DIR}/googletest/include ${GTest_SOURCE_DIR}/googlemock/include)
    endif()
  else()
    # Some compiler sources include GTest headers without linking its target.
    get_target_property(P4C_GTEST_INCLUDE_DIRS GTest::gtest INTERFACE_INCLUDE_DIRECTORIES)
    include_directories(BEFORE SYSTEM ${P4C_GTEST_INCLUDE_DIRS})
  endif()
  message(STATUS "Done with setting up GoogleTest for P4C.")
endmacro()
