macro(p4c_obtain_protobuf)
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
    set(protobuf_MODULE_COMPATIBLE TRUE)
    find_package(Protobuf CONFIG)
    if(NOT Protobuf_FOUND)
      find_package(Protobuf REQUIRED)
    endif()
    # Protobuf sets the protoc binary to a generator expression, which are problematic. They are
    # problematic because they are only evaluated at build time. However, we may have scripts that
    # depend on the actual build time during the configure phase. Hard code a custom binary instead.
    find_program(PROTOC_BINARY protoc)

    if(ENABLE_PROTOBUF_STATIC)
      set(CMAKE_FIND_LIBRARY_SUFFIXES ${SAVED_CMAKE_FIND_LIBRARY_SUFFIXES})
    endif()
    # While Protobuf_LIBRARY is already defined by find_package(Protobuf) because we set protobuf_MODULE_COMPATIBLE,
    # it only contains the path to the protobuf shared object, not libraries Protobuf depends on, such as abseil for Protobuf >= 22.
    # See https://github.com/p4lang/p4c/issues/4316
    set(Protobuf_LIBRARY "protobuf::libprotobuf")
  else()
    set(P4C_PROTOBUF_VERSION "25.3")
    message("Fetching Protobuf version ${P4C_PROTOBUF_VERSION} for P4C...")

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

    # Exclude Protobuf from the main make install step. We only want to use it locally.
    fetchcontent_makeavailable_but_exclude_install(protobuf)

    # Protobuf and protoc source code may trigger warnings which we need to ignore.
    set_target_properties(libprotobuf PROPERTIES COMPILE_FLAGS "-Wno-error")
    set_target_properties(libprotoc PROPERTIES COMPILE_FLAGS "-Wno-error")

    # Add the Protobuf directory to our module path for module builds.
    list(APPEND CMAKE_MODULE_PATH "${protobuf_BINARY_DIR}/cmake/protobuf")

    # Set some Protobuf variables manually until we are able to call FindPackage directly.
    set(Protobuf_FOUND TRUE)
    set(Protobuf_LIBRARY "protobuf::libprotobuf")
    set(Protobuf_PROTOC_LIBRARY "protobuf::libprotoc")
    set(Protobuf_PROTOC_EXECUTABLE $<TARGET_FILE:protoc>)
    # Protobuf sets the protoc binary to a generator expression, which are problematic. They are
    # problematic because they are only evaluated at build time. However, we may have scripts that
    # depend on the actual build time during the configure phase. Hard code a custom binary which we
    # can use in addition to the generator expression.
    # TODO: Maybe we can improve these scripts somehow?
    set(PROTOC_BINARY ${protobuf_BINARY_DIR}/protoc)
    list(APPEND Protobuf_INCLUDE_DIRS "${protobuf_SOURCE_DIR}/src/")

    # Reset temporary variable modifications.
    set(CMAKE_UNITY_BUILD ${CMAKE_UNITY_BUILD_PREV})
    set(FETCHCONTENT_QUIET ${FETCHCONTENT_QUIET_PREV})
    set(CMAKE_POSITION_INDEPENDENT_CODE ${CMAKE_POSITION_INDEPENDENT_CODE_PREV})
  endif()

  # Protobuf does not seem to set Protobuf_INCLUDE_DIR correctly, but we need this variable for
  # generating code with protoc. Instead, we rely on Protobuf_INCLUDE_DIRS and generate a custom
  # utility variable for includes.
  set(PROTOBUF_PROTOC_INCLUDES)
  foreach(dir ${Protobuf_INCLUDE_DIRS})
    list(APPEND PROTOBUF_PROTOC_INCLUDES "-I${dir}")
  endforeach()

  message("Done with setting up Protobuf for P4C.")
endmacro(p4c_obtain_protobuf)
