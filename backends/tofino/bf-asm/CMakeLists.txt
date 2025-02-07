# Copyright (C) 2024 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License.  You may obtain
# a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
#
# SPDX-License-Identifier: Apache-2.0

# # # #### Tofino assembler
project(BFASM)

MESSAGE("-- Adding bf-asm")

OPTION(ASAN_ENABLED "Enable ASAN checks" OFF)

set (BFASM_LIB_DEPS p4ctoolkit ${P4C_LIB_DEPS})
set (BFASM_GEN_DIR ${BFASM_BINARY_DIR}/gen)

# other required libraries
include (CheckLibraryExists)
# check includes
include (CheckIncludeFile)
check_include_file (execinfo.h HAVE_EXECINFO_H)
check_include_file (ucontext.h HAVE_UCONTEXT_H)

set(CMAKE_CXX_FLAGS "")  # clear CXX_FLAGS
# TODO: Fix build warnings with -Wall and enable it.
# add_cxx_compiler_option ("-Wall")
# add_cxx_compiler_option ("-Wextra")
# add_cxx_compiler_option ("-Wno-unused")
# add_cxx_compiler_option ("-Wno-unused-parameter")
# add_cxx_compiler_option ("-Wno-pragmas")
# add_cxx_compiler_option ("-Wno-unknown-pragmas")
add_cxx_compiler_option ("-Wno-overloaded-virtual")
add_cxx_compiler_option ("-Wno-deprecated")
if (${CMAKE_SYSTEM_PROCESSOR} MATCHES i386|i586|i686)
  # on 32-bit platforms we get a lot of warnings when using the error macros
  add_cxx_compiler_option("-Wno-write-strings")
endif()
if (ENABLE_BAREFOOT_INTERNAL)
  add_definitions("-DBAREFOOT_INTERNAL=1")
endif()

message(STATUS "P4C ${P4C_SOURCE_DIR}")
macro(get_schema_version schema_file schema_var)
  execute_process(
    COMMAND python3 -c "from ${schema_file} import get_schema_version;print(get_schema_version(), end='', flush=True)"
    OUTPUT_VARIABLE __schema_version
    RESULT_VARIABLE __schema_errcode
    ERROR_VARIABLE __schema_errstr
    WORKING_DIRECTORY ${BFN_P4C_SOURCE_DIR}/compiler_interfaces/schemas)
  if (${__schema_errcode})
    MESSAGE(FATAL_ERROR "Error retrieving ${schema_file} version ${__schema_errstr}")
  endif()
  set(${schema_var} ${__schema_version})
endmacro(get_schema_version)
# Now force cmake to rerun if any of the files that we depend on versions for
# change: context and manifest for now
# We generate a pair of dummy dependency files will be ignored
set(SCHEMA_FILES
  ${BFN_P4C_SOURCE_DIR}/compiler_interfaces/schemas/context_schema.py
  ${BFN_P4C_SOURCE_DIR}/compiler_interfaces/schemas/manifest_schema.py
  ${BFN_P4C_SOURCE_DIR}/compiler_interfaces/schemas/phv_schema.py
  ${BFN_P4C_SOURCE_DIR}/compiler_interfaces/schemas/power_schema.py
  ${BFN_P4C_SOURCE_DIR}/compiler_interfaces/schemas/resources_schema.py
  )
foreach (f ${SCHEMA_FILES})
  configure_file(${f} ${CMAKE_BINARY_DIR}/${f}.dep)
endforeach()

get_schema_version(context_schema CONTEXT_SCHEMA_VERSION)
MESSAGE(STATUS "Found context schema version ${CONTEXT_SCHEMA_VERSION}")
add_definitions("-DCONTEXT_SCHEMA_VERSION=\"${CONTEXT_SCHEMA_VERSION}\"")

# ASAN CHECKS
if (ASAN_ENABLED)
  # force this set of flags only
  set (CMAKE_CXX_FLAGS "-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -fno-optimize-sibling-calls -g -O1")
endif()

# json_diff
set (JSONDIFF_SOURCES json_diff.cpp json.cpp fdstream.cpp)

# bfdumpbin
set (BFDUMPBIN_SOURCES bfdumpbin.cpp fdstream.cpp json.cpp bson.cpp)

# bfdis
set (BFDIS_SOURCES bfdis.cpp disasm.cpp fdstream.cpp)

# reflow
set (REFLOW_SOURCES reflow.cpp)

# b2j
set (B2J_SOURCES b2j.cpp json.cpp bson.cpp)

# j2b
set (J2B_SOURCES j2b.cpp json.cpp bson.cpp)

# mksizes
set (MKSIZES_SOURCES mksizes.cpp)

# json_diff
add_executable (json_diff ${JSONDIFF_SOURCES})

# bfdumpbin
add_executable (bfdumpbin ${BFDUMPBIN_SOURCES})

# reflow
add_executable (reflow ${REFLOW_SOURCES})

# b2j
add_executable (b2j ${B2J_SOURCES})

# j2b
add_executable (j2b ${J2B_SOURCES})

# mksizes
add_executable (mksizes ${MKSIZES_SOURCES})

set (BFAS_COMMON_SOURCES
  action_bus.cpp
  action_table.cpp
  asm-types.cpp
  atcam_match.cpp
  attached_table.cpp
  bfas.cpp
  bson.cpp
  counter.cpp
  crash.cpp
  deparser.cpp
  depositfield.cpp
  dynhash.cpp
  error_mode.cpp
  exact_match.cpp
  exename.cpp
  flexible_headers.cpp
  gateway.cpp
  hash_action.cpp
  hash_dist.cpp
  hashexpr.cpp
  idletime.cpp
  input_xbar.cpp
  instruction.cpp
  json.cpp
  match_table.cpp
  meter.cpp
  misc.cpp
  p4_table.cpp
  parser-tofino-jbay.cpp
  phase0.cpp
  phv.cpp
  primitives.cpp
  proxy_hash.cpp
  salu_inst.cpp
  selection.cpp
  sram_match.cpp
  stage.cpp
  stateful.cpp
  synth2port.cpp
  tables.cpp
  target.cpp
  ternary_match.cpp
  top_level.cpp
  ubits.cpp
  vector.c
  widereg.cpp
  # FIXME: This should be a library.
  ${BFN_P4C_SOURCE_DIR}/bf-utils/dynamic_hash/dynamic_hash.cpp
  ${BFN_P4C_SOURCE_DIR}/bf-utils/dynamic_hash/bfn_hash_algorithm.cpp
  )


BISON_TARGET (asm-parse asm-parse.ypp ${BFASM_GEN_DIR}/asm-parse.cpp VERBOSE)

add_custom_command(OUTPUT ${BFASM_GEN_DIR}/uptr_sizes.h
  COMMAND ${CMAKE_COMMAND} -E make_directory ${BFASM_GEN_DIR}
  COMMAND ${BFASM_BINARY_DIR}/mksizes > ${BFASM_GEN_DIR}/uptr_sizes.h)
add_custom_target(bfasm_uptr DEPENDS mksizes ${BFASM_GEN_DIR}/uptr_sizes.h)

add_custom_command(OUTPUT ${BFASM_GEN_DIR}/lex-yaml.c
  COMMAND ${FLEX_EXECUTABLE} -t ${BFASM_SOURCE_DIR}/lex-yaml.l > ${BFASM_GEN_DIR}/lex-yaml.c
  DEPENDS ${BFASM_SOURCE_DIR}/lex-yaml.l
  COMMENT "Generating lex-yaml.cpp")
add_custom_target(bfasm_yaml DEPENDS ${BFASM_GEN_DIR}/lex-yaml.c)
add_dependencies(bfasm_yaml bfasm_uptr)

set (BFAS_GEN_SOURCES
  ${BFASM_GEN_DIR}/asm-parse.cpp
  ${BFASM_GEN_DIR}/uptr_sizes.h
)

set (BFASM_WALLE ${BFASM_SOURCE_DIR}/walle/walle.py)
set (WALLE_SOURCES
      ${BFASM_SOURCE_DIR}/walle/chip.py
      ${BFASM_SOURCE_DIR}/walle/csr.py
      ${BFASM_SOURCE_DIR}/walle/walle.py)

add_subdirectory (tofino)
set (HAVE_TOFINO 1)
set (BFASM_LIBS regs_tofino)
add_subdirectory (jbay)
set (HAVE_JBAY 1)
set (BFASM_LIBS ${BFASM_LIBS} regs_jbay)

# Other configuration files that need to be generated
configure_file ("${BFASM_SOURCE_DIR}/cmake/config.h.cmake" "${BFASM_BINARY_DIR}/config.h")

set_source_files_properties (${BFAS_GEN_SOURCES} ${BFASM_GEN_DIR}/lex-yaml.c PROPERTIES GENERATED TRUE)

set (BFAS_SOURCES ${BFAS_COMMON_SOURCES} ${BFAS_GEN_SOURCES}
  ${BFAS_TOFINO_SRCS}
  ${BFAS_JBAY_SRCS}
)

# bfdis
if (ENABLE_GTESTS)
  # FIXME -- bfdis depends on bfas_lib which is only built if GTESTS are enabled.  So for
  # now we only enable bfdis with ENABLE_GTESTS.  Should fix to use bfas_lib for bfas
  # rather than building separately, so it will always be anbled.
  add_executable (bfdis ${BFDIS_SOURCES})
  target_link_libraries (bfdis bfas_lib ${BFASM_LIBS} ${BFASM_LIB_DEPS})
endif()


# set_source_files_properties(${BFAS_SOURCES} PROPERTIES COMPILE_FLAGS ${BFASM_CXX_FLAGS})
# Remove compiler flag that is C++ only for vector.c
# string(REPLACE "-Wno-overloaded-virtual" "" vector_c_flags ${BFASM_CXX_FLAGS})
# set_source_files_properties(vector.c PROPERTIES COMPILE_FLAGS ${vector_c_flags})
add_executable (bfas ${BFAS_SOURCES})
target_compile_options(bfas PRIVATE -std=gnu++17)  # Enable extensions for bfas.
target_link_libraries (bfas ${BFASM_LIBS} ${BFASM_LIB_DEPS})
add_dependencies(bfas bfasm_yaml)

install (TARGETS bfas
  RUNTIME DESTINATION bin)
# Link bfas into the p4c binary folder.
add_custom_target(linkbfas
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${BFASM_BINARY_DIR}/bfas ${P4C_BINARY_DIR}/bfas
  )
add_dependencies(linkbfas bfas)
add_dependencies(p4c_driver linkbfas)


add_custom_target(check-asm
  COMMAND export BUILDDIR=${BFASM_GEN_DIR} && cd ${BFASM_SOURCE_DIR}/test &&
          ./runtests asm/*.tfa asm/*.jba asm/*.bfa asm/*.stf)

# This is broken and it doesn't make sense to use p4 tests here anyways
# add_custom_target(check-all-asm
#   COMMAND export BUILDDIR=${BFASM_GEN_DIR} && cd ${BFASM_SOURCE_DIR}/test &&
#           ./runtests -f asm/*.tfa *.p4 mau/*.p4
#   COMMAND export BUILDDIR=${BFASM_GEN_DIR} && cd ${BFASM_SOURCE_DIR}/test &&
#           ./runtests -f -b stf/*.stf brig/*.p4)

add_custom_target(check-sanity
  COMMAND export BUILDDIR=${BFASM_GEN_DIR} && cd ${BFASM_SOURCE_DIR}/test &&
  ./runtests *.p4)

string(CONFIGURE "/^DECLARE_(ABSTRACT_)?TABLE_TYPE\(([a-zA-Z0-9_]+)/2/c/" CTAGS_CXX_REGEXP @ONLY)
add_custom_target(ctags-asm
  COMMAND ctags -R -I VECTOR --exclude=test --exclude=submodules
              "--regex-C++=${CTAGS_CXX_REGEXP}"
  COMMENT "Generating ctags")

# Build the bf-asm test suite
add_subdirectory(test)

if (ENABLE_GTESTS)
  # TODO Components need to be built, once, into intermediate libraries.
  # These lib would then be linked to unit-tests and also linked into larger components/executable.
  # The exact shape, size, hierarchy of components is to be decided.
  # For now we will allow the source to be built twice, once for 'bfas', and once for
  # gtest/CMakeList.txt as a single monolithic component 'bfas_lib'.
  # TODO ASAP refactor bfas.cpp, moving main() into its own file.
  #           add_executable (bfas asm_main.cpp)
  #           target_link_libraries (bfas bfas_lib ${BFASM_LIBS} ${BFASM_LIB_DEPS})

  add_library (bfas_lib ${BFAS_SOURCES})
  target_compile_definitions(bfas_lib PRIVATE BUILDING_FOR_GTEST)  # removes main()
  target_link_libraries (bfas_lib PRIVATE ${BFASM_LIBS} ${BFASM_LIB_DEPS})

  set(BFAS_GTEST_SOURCES
    gtest/gtestasm.cpp
    gtest/asm-types.cpp
    gtest/depositfield.cpp
    gtest/gateway.cpp
    gtest/hashexpr.cpp
    gtest/mirror.cpp
    gtest/parser-test.cpp
    gtest/register-matcher.h
    gtest/register-matcher.cpp
    )

  # Do not use a unity build for gtestasm (for now).
  set_source_files_properties (${BFAS_GTEST_SOURCES} PROPERTIES SKIP_UNITY_BUILD_INCLUSION TRUE)

  add_executable (gtestasm ${BFAS_GTEST_SOURCES} ${BFP4C_SOURCES})
  target_link_libraries (gtestasm PRIVATE bfas_lib gtest)
  target_compile_options (gtestasm PRIVATE -Wall -Wextra -ggdb -O3
                          -Wno-unused-parameter -Wno-sign-compare)

  # Add to CTests - but this is in the BFASM project viz build/bf-asm, not build/p4c
  add_test (NAME gtestasm COMMAND gtestasm WORKING_DIRECTORY ${P4C_BINARY_DIR})
  set_tests_properties (gtestasm PROPERTIES LABELS "gtest")
endif ()
