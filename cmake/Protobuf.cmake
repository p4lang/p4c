# SPDX-FileCopyrightText: 2023 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

macro(p4c_obtain_protobuf)
  set(P4C_PROTOBUF_VERSION 25.3)
  p4c_dependency_compat(P4C_USE_PREINSTALLED_PROTOBUF Protobuf)
  if(ENABLE_PROTOBUF_STATIC)
    set(SAVED_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
  endif()
  if(APPLE)
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
      "CMAKE_UNITY_BUILD OFF"
      "CMAKE_POSITION_INDEPENDENT_CODE ON"
      "protobuf_BUILD_TESTS OFF"
      "protobuf_BUILD_PROTOC_BINARIES ON"
      # Linking with a local shared Protobuf may mix incompatible versions.
      "protobuf_BUILD_SHARED_LIBS OFF"
      "protobuf_INSTALL OFF"
      "protobuf_ABSL_PROVIDER package"
      "utf8_range_ENABLE_INSTALL OFF"
  )

  if(Protobuf_SOURCE_DIR)
    if(Protobuf_ADDED)
      foreach(target libprotobuf-lite libprotobuf libprotoc)
        target_compile_options(${target} PRIVATE "-Wno-error" "-w")
      endforeach()
    endif()
    # Test scripts need a concrete protoc path rather than a generator expression.
    set(Protobuf_PROTOC_EXECUTABLE ${Protobuf_BINARY_DIR}/protoc)
    include(${Protobuf_SOURCE_DIR}/cmake/protobuf-generate.cmake)
    set(Protobuf_INCLUDE_DIRS ${Protobuf_SOURCE_DIR}/src)
  else()
    # CPM exports targets, but not find_package's result variables or functions.
    find_package(Protobuf ${P4C_PROTOBUF_VERSION} CONFIG QUIET ${P4C_PROTOBUF_PATHS})
    if(NOT Protobuf_FOUND)
      find_package(Protobuf REQUIRED CONFIG ${P4C_PROTOBUF_PATHS})
      message(WARNING
        "Major Protobuf version does not match with the expected ${P4C_PROTOBUF_VERSION} version."
        " You may experience compatibility problems.")
    endif()
    find_program(Protobuf_PROTOC_EXECUTABLE protoc)
  endif()
  if(ENABLE_PROTOBUF_STATIC)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${SAVED_CMAKE_FIND_LIBRARY_SUFFIXES})
  endif()
  message(STATUS "Done with setting up Protobuf for P4C.")
endmacro()
