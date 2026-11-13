# SPDX-FileCopyrightText: 2023 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

macro(p4c_obtain_protobuf)
  set(P4C_PROTOBUF_VERSION 25.3)
  p4c_dependency_compat(P4C_USE_PREINSTALLED_PROTOBUF Protobuf)
  if(APPLE)
    # Homebrew uses different installation prefixes on Intel and Apple Silicon.
    set(P4C_PROTOBUF_PATHS PATHS /usr/local/opt/protobuf /opt/homebrew/opt/protobuf)
  endif()
  string(JOIN " " P4C_PROTOBUF_FIND_ARGUMENTS CONFIG ${P4C_Protobuf_FIND_ARGUMENTS} ${P4C_PROTOBUF_PATHS})
  p4c_find_package(
    NAME Protobuf
    URL https://github.com/protocolbuffers/protobuf/releases/download/v${P4C_PROTOBUF_VERSION}/protobuf-${P4C_PROTOBUF_VERSION}.tar.gz
    URL_HASH SHA256=d19643d265b978383352b3143f04c0641eea75a75235c111cc01a1350173180e
    FIND_PACKAGE_ARGUMENTS "${P4C_PROTOBUF_FIND_ARGUMENTS}"
    EXCLUDE_FROM_ALL YES
    SYSTEM YES
    OPTIONS
      # Protobuf's sources cannot be combined into unity translation units.
      "CMAKE_UNITY_BUILD OFF"
      "CMAKE_POSITION_INDEPENDENT_CODE ON"
      "protobuf_BUILD_TESTS OFF"
      "protobuf_BUILD_PROTOC_BINARIES ON"
      # Build a static library to avoid loading a different installed Protobuf at runtime.
      "protobuf_BUILD_SHARED_LIBS OFF"
      "protobuf_INSTALL OFF"
      # Reuse the Abseil targets that P4C has already selected.
      "protobuf_ABSL_PROVIDER package"
      "utf8_range_ENABLE_INSTALL OFF"
  )

  # SYSTEM covers headers used by P4C, not Protobuf's own compilation.
  # Adjust warnings only for targets added here, preserving a parent's settings.
  if(Protobuf_ADDED)
    foreach(target libprotobuf-lite libprotobuf libprotoc)
      target_compile_options(${target} PRIVATE "-Wno-error" "-w")
    endforeach()
  endif()

  if(Protobuf_SOURCE_DIR)
    # Test scripts are written at configure time, before generator expressions
    # such as $<TARGET_FILE:protoc> can be evaluated. Give them a concrete path.
    set(Protobuf_PROTOC_EXECUTABLE ${Protobuf_BINARY_DIR}/protoc)
    include(${Protobuf_SOURCE_DIR}/cmake/protobuf-generate.cmake)
    set(Protobuf_INCLUDE_DIRS ${Protobuf_SOURCE_DIR}/src)
  else()
    # Repeat discovery in this scope to obtain Protobuf's variables and helpers.
    # Its package checks major-version compatibility rather than a minimum
    # version, so retain the fallback for other installed releases.
    find_package(Protobuf ${P4C_PROTOBUF_VERSION} CONFIG QUIET ${P4C_PROTOBUF_PATHS})
    if(NOT Protobuf_FOUND)
      find_package(Protobuf REQUIRED CONFIG ${P4C_PROTOBUF_PATHS})
      message(WARNING
        "Major Protobuf version does not match with the expected ${P4C_PROTOBUF_VERSION} version."
        " You may experience compatibility problems.")
    endif()
    find_program(Protobuf_PROTOC_EXECUTABLE protoc)
  endif()
  message(STATUS "Done with setting up Protobuf for P4C.")
endmacro()
