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
    find_package(Protobuf CONFIG)
    if(NOT Protobuf_FOUND)
      find_package(Protobuf REQUIRED)
    endif()
    # Backwards compatibility
    # Define camel case versions of input variables
    foreach(UPPER
        PROTOBUF_SRC_ROOT_FOLDER
        PROTOBUF_IMPORT_DIRS
        PROTOBUF_DEBUG
        PROTOBUF_LIBRARY
        PROTOBUF_PROTOC_LIBRARY
        PROTOBUF_INCLUDE_DIR
        PROTOBUF_PROTOC_EXECUTABLE
        PROTOBUF_LIBRARY_DEBUG
        PROTOBUF_PROTOC_LIBRARY_DEBUG
        PROTOBUF_LITE_LIBRARY
        PROTOBUF_LITE_LIBRARY_DEBUG
        )
        if (DEFINED ${UPPER})
            string(REPLACE "PROTOBUF_" "Protobuf_" Camel ${UPPER})
            if (NOT DEFINED ${Camel})
                set(${Camel} ${${UPPER}})
            endif()
        endif()
    endforeach()
    # Protobuf sets the protoc binary to a generator expression, which are highly problematic.
    # They are problematic because they are basically evaluated at build time.
    # However, we may have scripts that depend on the actual build time during the configure phase.
    # Hard code a custom binary instead.
    find_program(PROTOC_BINARY protoc)

    if(ENABLE_PROTOBUF_STATIC)
      set(CMAKE_FIND_LIBRARY_SUFFIXES ${SAVED_CMAKE_FIND_LIBRARY_SUFFIXES})
    endif()
  else()
    # Google introduced another breaking change with protobuf 22.x by adding abseil as a new
    # dependency. https://protobuf.dev/news/2022-08-03/#abseil-dep We do not want abseil, so we stay
    # with 21.x for now.
    set(P4C_PROTOBUF_VERSION "21.12")
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
    # This is necessary to be able to call FindPackage.
    set(protobuf_INSTALL ON CACHE BOOL "Install Protobuf")
    # Derive the target architecture in order to build the right architecture.
    if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "aarch64" OR CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL
                                                         "arm64"
    )
      set(protobuf_ARCH "aarch_64")
    elseif(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL
                                                            "amd64"
    )
      set(protobuf_ARCH "x86_64")
    else()
      message(FATAL_ERROR "Unsupported host architecture `${CMAKE_HOST_SYSTEM_PROCESSOR}`")
    endif()

    fetchcontent_declare(
      protobuf
      URL https://github.com/protocolbuffers/protobuf/releases/download/v${P4C_PROTOBUF_VERSION}/protobuf-cpp-3.${P4C_PROTOBUF_VERSION}.tar.gz
      URL_HASH SHA256=4eab9b524aa5913c6fffb20b2a8abf5ef7f95a80bc0701f3a6dbb4c607f73460
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
    # Protobuf sets the protoc binary to a generator expression, which are highly problematic.
    # They are problematic because they are basically evaluated at build time.
    # However, we may have scripts that depend on the actual build time during the configure phase.
    # Hard code a custom binary instead.
    set(PROTOC_BINARY ${protobuf_BINARY_DIR}/protoc)
    set(Protobuf_INCLUDE_DIR "${protobuf_SOURCE_DIR}/src/")

    # Reset temporary variable modifications.
    set(CMAKE_UNITY_BUILD ${CMAKE_UNITY_BUILD_PREV})
    set(FETCHCONTENT_QUIET ${FETCHCONTENT_QUIET_PREV})
    set(CMAKE_POSITION_INDEPENDENT_CODE ${CMAKE_POSITION_INDEPENDENT_CODE_PREV})
  endif()


  message("Done with setting up Protobuf for P4C.")
endmacro(p4c_obtain_protobuf)
