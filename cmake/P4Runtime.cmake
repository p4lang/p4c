# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

function(p4c_obtain_p4runtime)
  p4c_dependency_compat(P4C_USE_PREINSTALLED_P4RUNTIME p4runtime)
  set(P4C_P4RUNTIME_FIND_ARGUMENTS "CONFIG COMPONENTS cpp ${P4C_p4runtime_FIND_ARGUMENTS}")
  if(CPM_LOCAL_PACKAGES_ONLY)
    # Fail during discovery if the installed package lacks C++ bindings.
    string(APPEND P4C_P4RUNTIME_FIND_ARGUMENTS " REQUIRED")
  endif()
  p4c_find_package(
    NAME p4runtime
    VERSION 1.6.0
    # P4Runtime 1.6.0 with CMake packaging.
    URL https://github.com/p4lang/p4runtime/archive/0bea6a3be25d6f852615a8d6b55fd68dfd624350.tar.gz
    URL_HASH SHA256=e64fac0ce4abf47aa0e2ed9920473edbba67aa1c79767f198f8606a8beea986e
    FIND_PACKAGE_ARGUMENTS "${P4C_P4RUNTIME_FIND_ARGUMENTS}"
    EXCLUDE_FROM_ALL YES
    SYSTEM YES
    OPTIONS
      "CMAKE_UNITY_BUILD OFF"
      "P4RUNTIME_BUILD_CPP ON"
      "P4RUNTIME_BUILD_GRPC OFF"
      "P4RUNTIME_INSTALL OFF"
  )
  if(p4runtime_SOURCE_DIR)
    set(proto_dir "${p4runtime_SOURCE_DIR}/proto")
    if(p4runtime_ADDED)
      # Generated Protobuf code is outside P4C's warning policy.
      target_compile_options(p4runtime_cpp PRIVATE
        -Wno-unused-parameter
        -Wno-array-bounds
        -Wno-error
      )
    endif()
  else()
    # Repeat discovery in this scope to obtain p4runtime_PROTO_DIR.
    find_package(p4runtime 1.6 CONFIG REQUIRED COMPONENTS cpp)
    set(proto_dir "${p4runtime_PROTO_DIR}")
  endif()
  if(NOT TARGET p4runtime::cpp)
    message(FATAL_ERROR "P4Runtime must provide the p4runtime::cpp target")
  endif()

  # Backend schemas and Python bindings still need the .proto import directory.
  # Refresh the old cache entry when switching dependency providers.
  set(P4RUNTIME_STD_DIR "${proto_dir}" CACHE INTERNAL "Path to P4Runtime schemas")
  set(P4RUNTIME_STD_DIR "${proto_dir}" PARENT_SCOPE)
  message(STATUS "P4Runtime schemas: ${proto_dir}")
endfunction()
