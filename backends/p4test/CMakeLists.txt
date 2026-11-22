# SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
# Copyright 2013-present Barefoot Networks, Inc.
#
# SPDX-License-Identifier: Apache-2.0

# Makefile for a fake backend that is used for testing the P4-16 front-end.

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.h.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/version.h" @ONLY)

set (P4TEST_SRCS
  p4test.cpp
  midend.cpp
  )
set (P4TEST_HDRS
  p4test.h
  midend.h
  )


add_executable(p4test ${P4TEST_SRCS} ${EXTENSION_P4_14_CONV_SOURCES})
target_link_libraries (p4test ${P4C_LIBRARIES} ${P4C_LIB_DEPS})
add_dependencies(p4test ir-generated frontend)

install (TARGETS p4test
  RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})

file(RELATIVE_PATH
  CURRENT_BINARY_DIR_PATH_REL
  ${P4C_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

file(RELATIVE_PATH
  P4C_BINARY_DIR_PATH_REL
  ${CMAKE_CURRENT_BINARY_DIR}
  ${P4C_BINARY_DIR}
)

# hack to get around the fact that the test scripts expect the backend
# binary to be in the top level directory. This should go away when we
# remove automake and fix the scripts.
add_custom_target(linkp4test
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CURRENT_BINARY_DIR_PATH_REL}/p4test ${P4C_BINARY_DIR}/p4test
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${P4C_BINARY_DIR_PATH_REL}/p4include ${CMAKE_CURRENT_BINARY_DIR}/p4include
  DEPENDS update_includes
  )
add_dependencies(p4c_driver linkp4test)

if (SUPPORT_P4_14)
add_custom_target(linkp414test
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${P4C_BINARY_DIR_PATH_REL}/p4_14include ${CMAKE_CURRENT_BINARY_DIR}/p4_14include
  DEPENDS update_includes
  )
add_dependencies(p4c_driver linkp414test)
endif()
