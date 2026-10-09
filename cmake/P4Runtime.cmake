# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

macro(p4c_obtain_p4runtime)
  set(P4C_P4RUNTIME_FIND_ARGUMENTS MODULE)
  if(CPM_LOCAL_PACKAGES_ONLY)
    string(APPEND P4C_P4RUNTIME_FIND_ARGUMENTS " REQUIRED")
  endif()
  p4c_find_package(
    NAME p4runtime
    VERSION 1.6.0
    # Test https://github.com/p4lang/p4runtime/pull/642 until it lands upstream.
    GIT_REPOSITORY https://github.com/fruffy/p4runtime.git
    GIT_TAG 5fb67ff78ba16674a38a7c41f2ddcb5824a6b667
    FIND_PACKAGE_ARGUMENTS "${P4C_P4RUNTIME_FIND_ARGUMENTS}"
    EXCLUDE_FROM_ALL YES
    SYSTEM YES
    # controlplane-gen generates C++ and Python bindings with P4C's Protobuf.
    OPTIONS
      "P4RUNTIME_BUILD_CPP OFF"
      "P4RUNTIME_BUILD_GRPC OFF"
      "P4RUNTIME_INSTALL OFF"
  )
  if(p4runtime_SOURCE_DIR)
    set(P4RUNTIME_STD_DIR "${p4runtime_SOURCE_DIR}/proto")
  else()
    # CPM exports package registration, but not find_package's result variables.
    find_package(p4runtime REQUIRED MODULE)
    set(P4RUNTIME_STD_DIR "${p4runtime_PROTO_DIR}")
  endif()
  # Refresh the old internal cache entry when switching between source and
  # installed schemas in the same build directory.
  set(P4RUNTIME_STD_DIR "${P4RUNTIME_STD_DIR}" CACHE INTERNAL "Path to P4Runtime schemas")
  message(STATUS "P4Runtime schemas: ${P4RUNTIME_STD_DIR}")
endmacro()
