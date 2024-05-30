macro(p4c_obtain_protobuf)
  set(P4C_PROTOBUF_VERSION 25.3)
  option(
    P4C_USE_PREINSTALLED_PROTOBUF
    "Look for a preinstalled version of Protobuf in the system instead of installing a prebuilt binary using FetchContent."
    OFF
  )

  # If P4C_USE_PREINSTALLED_PROTOBUF is ON just try to find a preinstalled version of Protobuf.
  if(P4C_USE_PREINSTALLED_PROTOBUF)
    if(ENABLE_PROTOBUF_STATIC)
      set(SAVED_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
      set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
    endif()
    # For MacOS, we may need to look for Protobuf in additional folders.
    if(APPLE)
      set(P4C_PROTOBUF_PATHS PATHS /usr/local/opt/protobuf /opt/homebrew/opt/protobuf)
    endif()
    # We do not set a minimum version here because Protobuf does not accept mismatched major versions.
    # We recommend the current P4C_PROTOBUF_VERSION.
    find_package(Protobuf ${P4C_PROTOBUF_VERSION} CONFIG ${P4C_PROTOBUF_PATHS})
    if(NOT Protobuf_FOUND)
      find_package(Protobuf REQUIRED CONFIG ${P4C_PROTOBUF_PATHS})
      message(
        WARNING
          "Major Protobuf version does not match with the expected ${P4C_PROTOBUF_VERSION} version."
          " You may experience compatibility problems."
      )
    endif()

    # Protobuf sets the protoc binary to a generator expression "$<TARGET_FILE:protoc>", but we many
    # not be able to use this generator expression in some text-based test scripts. The reason is
    # that protoc is only evaluated at build time, not during generation of the test scripts. TODO:
    # Maybe we can improve these scripts somehow?
    find_program(Protobuf_PROTOC_EXECUTABLE protoc)

    if(ENABLE_PROTOBUF_STATIC)
      set(CMAKE_FIND_LIBRARY_SUFFIXES ${SAVED_CMAKE_FIND_LIBRARY_SUFFIXES})
    endif()
  else()
    message(STATUS "Fetching Protobuf version ${P4C_PROTOBUF_VERSION} for P4C...")

    # Unity builds do not work for Protobuf...
    set(CMAKE_UNITY_BUILD_PREV ${CMAKE_UNITY_BUILD})
    set(CMAKE_UNITY_BUILD OFF)
    # Print out download state while setting up Protobuf.
    set(FETCHCONTENT_QUIET_PREV ${FETCHCONTENT_QUIET})
    set(FETCHCONTENT_QUIET OFF)
    # Build Protobuf with position-independent code.
    set(CMAKE_POSITION_INDEPENDENT_CODE_PREV ${CMAKE_POSITION_INDEPENDENT_CODE})
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)

    set(protobuf_BUILD_TESTS OFF CACHE BOOL "Build tests.")
    set(protobuf_BUILD_PROTOC_BINARIES ON CACHE BOOL "Build libprotoc and protoc compiler.")
    # Only ever build the static library. It is not safe to link with a local dynamic version.
    set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "Build Shared Libraries")
    # Exclude Protobuf from the main make install step. We only want to use it locally.
    set(protobuf_INSTALL OFF CACHE BOOL "Install Protobuf")
    set(protobuf_ABSL_PROVIDER "package" CACHE STRING "Use system-provided abseil")
    set(protobuf_BUILD_EXPORT OFF)
    set(utf8_range_ENABLE_INSTALL OFF)

    fetchcontent_declare(
      protobuf
      URL https://github.com/protocolbuffers/protobuf/releases/download/v${P4C_PROTOBUF_VERSION}/protobuf-${P4C_PROTOBUF_VERSION}.tar.gz
      URL_HASH SHA256=d19643d265b978383352b3143f04c0641eea75a75235c111cc01a1350173180e
      USES_TERMINAL_DOWNLOAD TRUE
      GIT_PROGRESS TRUE
    )
    fetchcontent_makeavailable(protobuf)

    # Protobuf and protoc source code may trigger warnings which we ignore.
    set_target_properties(libprotobuf-lite PROPERTIES COMPILE_FLAGS "-Wno-error -w")
    set_target_properties(libprotobuf PROPERTIES COMPILE_FLAGS "-Wno-error -w")
    set_target_properties(libprotoc PROPERTIES COMPILE_FLAGS "-Wno-error -w")

    # Set some Protobuf variables manually until we are able to call FindPackage directly. This
    # should be possible with CMake 3.24. Protobuf sets the protoc binary to a generator expression
    # "$<TARGET_FILE:protoc>", but we many not be able to use this generator expression in some
    # text-based test scripts. The reason is that protoc is only evaluated at build time, not during
    # generation of the test scripts. TODO: Maybe we can improve these scripts somehow?
    set(Protobuf_PROTOC_EXECUTABLE ${protobuf_BINARY_DIR}/protoc)
    include(${protobuf_SOURCE_DIR}/cmake/protobuf-generate.cmake)
    # Protobuf does not seem to set Protobuf_INCLUDE_DIRS correctly when used as a module, but we
    # need this variable for generating code.
    list(APPEND Protobuf_INCLUDE_DIRS "${protobuf_SOURCE_DIR}/src/")

    # Reset temporary variable modifications.
    set(CMAKE_UNITY_BUILD ${CMAKE_UNITY_BUILD_PREV})
    set(FETCHCONTENT_QUIET ${FETCHCONTENT_QUIET_PREV})
    set(CMAKE_POSITION_INDEPENDENT_CODE ${CMAKE_POSITION_INDEPENDENT_CODE_PREV})
  endif()

  message(STATUS "Done with setting up Protobuf for P4C.")
endmacro(p4c_obtain_protobuf)
