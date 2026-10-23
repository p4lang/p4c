# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

function(p4c_obtain_p4runtime)
  option(
    P4C_USE_PREINSTALLED_P4RUNTIME
    "Use an installed P4Runtime CMake package instead of fetching it."
    OFF
  )

  if(P4C_USE_PREINSTALLED_P4RUNTIME)
    find_package(p4runtime 1.6 CONFIG REQUIRED COMPONENTS cpp)
    set(proto_dir "${p4runtime_PROTO_DIR}")
  else()
    # These settings are local to this function and its subdirectories.
    set(P4RUNTIME_BUILD_CPP ON)
    set(P4RUNTIME_BUILD_GRPC OFF)
    set(P4RUNTIME_INSTALL OFF)
    set(CMAKE_UNITY_BUILD OFF)
    set(FETCHCONTENT_QUIET OFF)

    FetchContent_Declare(
      p4runtime
      # P4Runtime 1.6.0 with CMake packaging: p4lang/p4runtime#642.
      URL https://github.com/p4lang/p4runtime/archive/0bea6a3be25d6f852615a8d6b55fd68dfd624350.tar.gz
      URL_HASH SHA256=e64fac0ce4abf47aa0e2ed9920473edbba67aa1c79767f198f8606a8beea986e
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(p4runtime)
    set(proto_dir "${p4runtime_SOURCE_DIR}/proto")

    # Generated Protobuf code is outside P4C's warning policy.
    target_compile_options(p4runtime_cpp PRIVATE
      -Wno-unused-parameter
      -Wno-array-bounds
      -Wno-error
    )
  endif()

  # Backend schemas and Python bindings still need the .proto import directory.
  set(P4RUNTIME_STD_DIR "${proto_dir}" PARENT_SCOPE)
endfunction()
