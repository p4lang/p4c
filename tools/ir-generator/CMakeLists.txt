# SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
# Copyright 2013-present Barefoot Networks, Inc.
#
# SPDX-License-Identifier: Apache-2.0

set (TOOLSDIR, "${P4C_SOURCE_DIR}/tools")
set (GENTESTS, "${TOOLSDIR}/gen-tests.py")

set (IRGENERATOR_SRCS
  generator.cpp
  irclass.cpp
  methods.cpp
  type.cpp
  )

set (IRGENERATOR_HDRS
  ir-generator.h
  irclass.h
  type.h
  )

BISON_TARGET(IRgenParser ir-generator.ypp ${CMAKE_CURRENT_BINARY_DIR}/ir-generator.cpp)
FLEX_TARGET(IRgenLexer ir-generator-lex.l  ${CMAKE_CURRENT_BINARY_DIR}/ir-generator-lex.c)
ADD_FLEX_BISON_DEPENDENCY(IRgenLexer IRgenParser)

set (IRGENERATOR_GEN_SRCS
    ${BISON_IRgenParser_OUTPUTS}
  )

include_directories (${CMAKE_CURRENT_BINARY_DIR})
set_source_files_properties(${IRGENERATOR_GEN_SRCS} PROPERTIES OBJECT_DEPENDS ${FLEX_IRgenLexer_OUTPUTS})
add_executable (irgenerator ${IRGENERATOR_SRCS} ${IRGENERATOR_GEN_SRCS})
# Unconditionally set the output directory for the irgenerator to the path
# where the custom command that generates IR sources expects to find it.
# The generator expression $<BOOL:1> prevents IDE generators like Xcode
# from appending a per-configuration subdirectory to the specified path.
set_property(TARGET irgenerator PROPERTY RUNTIME_OUTPUT_DIRECTORY $<$<BOOL:1>:${IR_GENERATOR_DIRECTORY}>)
target_link_libraries (irgenerator p4ctoolkit ${P4C_LIB_DEPS} ${CMAKE_THREAD_LIBS_INIT})
