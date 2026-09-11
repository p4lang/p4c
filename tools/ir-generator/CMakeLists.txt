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

# Versions of CMake before 3.7.2 require a filename argument to BISON_TARGET's
# VERBOSE option. Later versions "support" it, but they generate a file with
# the same name automatically anyway, and then issue a warning because the
# file gets overwritten, so we need this code to avoid warning spew during
# compilation.
if (CMAKE_VERSION VERSION_LESS "3.7.2")
  BISON_TARGET(IRgenParser ir-generator.ypp ${CMAKE_CURRENT_BINARY_DIR}/ir-generator.cpp VERBOSE ${CMAKE_CURRENT_BINARY_DIR}//ir-generator.output)
else ()
  BISON_TARGET(IRgenParser ir-generator.ypp ${CMAKE_CURRENT_BINARY_DIR}/ir-generator.cpp VERBOSE COMPILE_FLAGS "-Werror=conflicts-sr -Werror=conflicts-rr")
endif ()
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
