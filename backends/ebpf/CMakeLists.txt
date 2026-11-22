# SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
# Copyright 2013-present Barefoot Networks, Inc.
#
# SPDX-License-Identifier: Apache-2.0

# CMakefile for the EBPF P4-16 back-end.
message(STATUS "Start configuring the eBPF back end")

if(NOT APPLE)
  # Fetch and declare the libbpf library. Print out download state while setting up libbpf.
  set(FETCHCONTENT_QUIET_PREV ${FETCHCONTENT_QUIET})
  set(FETCHCONTENT_QUIET OFF)
  fetchcontent_declare(
    bpfrepo
    URL https://github.com/libbpf/bpftool/releases/download/v7.5.0/bpftool-libbpf-v7.5.0-sources.tar.gz
    # URL_HASH SHA256=cc01a3a05d25e5978c20be7656f14eb8b6fcb120bb1c7e8041e497814fc273cb
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/runtime/contrib/bpftool
    USES_TERMINAL_DOWNLOAD TRUE
    GIT_PROGRESS TRUE
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  fetchcontent_makeavailable(bpfrepo)
  set(FETCHCONTENT_QUIET ${FETCHCONTENT_QUIET_PREV})
  # Check if we have already built the libbpf library.
  find_library(LIBBPF NAMES bpf HINTS "${CMAKE_CURRENT_SOURCE_DIR}/runtime/install/libbpf/")
  if (NOT LIBBPF)
    message("Building libbpf...")
    execute_process(
      COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/build_libbpf
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
  endif()
  message(STATUS "Done with setting up libbpf for P4C.")
endif()


configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.h.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/version.h" @ONLY)

set (P4C_EBPF_SRCS
  p4c-ebpf.cpp
  ebpfBackend.cpp
  ebpfProgram.cpp
  ebpfTable.cpp
  ebpfControl.cpp
  ebpfDeparser.cpp
  ebpfParser.cpp
  ebpfOptions.cpp
  target.cpp
  ebpfType.cpp
  codeGen.cpp
  ebpfModel.cpp
  midend.cpp
  lower.cpp
  psa/ebpfPsaGen.cpp
  psa/ebpfPipeline.cpp
  psa/ebpfPsaParser.cpp
  psa/ebpfPsaDeparser.cpp
  psa/ebpfPsaControl.cpp
  psa/ebpfPsaTable.cpp
  psa/backend.cpp
  psa/externs/ebpfPsaCounter.cpp
  psa/externs/ebpfPsaChecksum.cpp
  psa/externs/ebpfPsaDigest.cpp
  psa/externs/ebpfPsaHashAlgorithm.cpp
  psa/externs/ebpfPsaTableImplementation.cpp
  psa/externs/ebpfPsaRandom.cpp
  psa/externs/ebpfPsaRegister.cpp
  psa/externs/ebpfPsaMeter.cpp
)

set (P4C_EBPF_HDRS
  codeGen.h
  ebpfBackend.h
  ebpfControl.h
  ebpfDeparser.h
  ebpfModel.h
  ebpfObject.h
  ebpfProgram.h
  ebpfOptions.h
  ebpfParser.h
  ebpfTable.h
  ebpfType.h
  midend.h
  target.h
  lower.h
  psa/backend.h
  psa/ebpfPsaGen.h
  psa/xdpHelpProgram.h
  psa/ebpfPipeline.h
  psa/ebpfPsaParser.h
  psa/ebpfPsaDeparser.h
  psa/ebpfPsaControl.h
  psa/ebpfPsaTable.h
  psa/externs/ebpfPsaCounter.h
  psa/externs/ebpfPsaChecksum.h
  psa/externs/ebpfPsaDigest.h
  psa/externs/ebpfPsaHashAlgorithm.h
  psa/externs/ebpfPsaTableImplementation.h
  psa/externs/ebpfPsaRegister.h
  psa/externs/ebpfPsaRandom.h
  psa/externs/ebpfPsaMeter.h
)

set (P4C_EBPF_DIST_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/../../p4include/ebpf_model.p4
  ${CMAKE_CURRENT_SOURCE_DIR}/../../p4include/xdp_model.p4
)

add_executable(p4c-ebpf ${P4C_EBPF_SRCS})
target_link_libraries (p4c-ebpf ${P4C_LIBRARIES} ${P4C_LIB_DEPS} backends-common)
add_dependencies(p4c-ebpf ir-generated frontend)

install (TARGETS p4c-ebpf
  RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})
install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/p4include
  DESTINATION ${P4C_ARTIFACTS_OUTPUT_DIRECTORY})

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
add_custom_target(linkp4cebpf
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CURRENT_BINARY_DIR_PATH_REL}/p4c-ebpf ${P4C_BINARY_DIR}/p4c-ebpf
  COMMAND ${CMAKE_COMMAND} -E make_directory ${P4C_BINARY_DIR}/p4include &&
          ${CMAKE_COMMAND} -E copy ${P4C_EBPF_DIST_HEADERS} ${P4C_BINARY_DIR}/p4include
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${P4C_BINARY_DIR_PATH_REL}/p4include ${CMAKE_CURRENT_BINARY_DIR}/p4include
  DEPENDS update_includes
  )

add_dependencies(p4c_driver linkp4cebpf)

# needs to be installed
# p4include_HEADERS += $(srcdir)/%reldir%/p4include/ebpf_model.p4
