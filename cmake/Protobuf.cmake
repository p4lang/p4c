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
    find_package(Protobuf 3.0.0 REQUIRED)
    if(ENABLE_PROTOBUF_STATIC)
      set(CMAKE_FIND_LIBRARY_SUFFIXES ${SAVED_CMAKE_FIND_LIBRARY_SUFFIXES})
    endif()
  else()
    # Google introduced another breaking change with protobuf 22.x by adding abseil as a new
    # dependency. https://protobuf.dev/news/2022-08-03/#abseil-dep We do not want abseil, so we stay
    # with 21.x for now.
    set(P4C_PROTOBUF_VERSION "21.12")
    message("Fetching Protobuf version ${P4C_PROTOBUF_VERSION} for P4C...")

    # Print out download state while setting up Protobuf.
    set(FETCHCONTENT_QUIET_PREV ${FETCHCONTENT_QUIET})
    set(FETCHCONTENT_QUIET OFF)
    set(protobuf_BUILD_TESTS OFF CACHE BOOL "Build tests.")
    set(protobuf_BUILD_PROTOC_BINARIES OFF CACHE BOOL "Build libprotoc and protoc compiler.")
    # Only ever build the static library. It is not safe to link with a local dynamic version.
    set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "Build Shared Libraries")
    # Unity builds do not work for Protobuf...
    set(SAVED_CMAKE_UNITY_BUILD ${CMAKE_UNITY_BUILD})
    set(CMAKE_UNITY_BUILD OFF)
    fetchcontent_declare(
      protobuf
      URL https://github.com/protocolbuffers/protobuf/releases/download/v${P4C_PROTOBUF_VERSION}/protobuf-cpp-3.${P4C_PROTOBUF_VERSION}.tar.gz
      URL_HASH SHA256=4eab9b524aa5913c6fffb20b2a8abf5ef7f95a80bc0701f3a6dbb4c607f73460
      USES_TERMINAL_DOWNLOAD TRUE
      GIT_PROGRESS TRUE
    )
    # Pull a different protoc binary for MacOS.
    # TODO: Should we build from scratch?
    if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
      fetchcontent_declare(
        protoc
        URL https://github.com/protocolbuffers/protobuf/releases/download/v${P4C_PROTOBUF_VERSION}/protoc-${P4C_PROTOBUF_VERSION}-osx-x86_64.zip
        USES_TERMINAL_DOWNLOAD TRUE
        GIT_PROGRESS TRUE
      )
    else()
      fetchcontent_declare(
        protoc
        URL https://github.com/protocolbuffers/protobuf/releases/download/v${P4C_PROTOBUF_VERSION}/protoc-${P4C_PROTOBUF_VERSION}-linux-x86_64.zip
        URL_HASH SHA256=3a4c1e5f2516c639d3079b1586e703fc7bcfa2136d58bda24d1d54f949c315e8
        USES_TERMINAL_DOWNLOAD TRUE
        GIT_PROGRESS TRUE
      )
    endif()

    fetchcontent_makeavailable(protoc)
    # Exclude Protobuf from the main make install step. We only want to use it locally.
    fetchcontent_makeavailable_but_exclude_install(protobuf)

    # Reset unity builds to the previous state...
    set(CMAKE_UNITY_BUILD ${SAVED_CMAKE_UNITY_BUILD})
    set(FETCHCONTENT_QUIET ${FETCHCONTENT_QUIET_PREV})

    set(PROTOBUF_PROTOC_EXECUTABLE ${protoc_SOURCE_DIR}/bin/protoc CACHE STRING
                                                                         "Protoc executable."
    )
    set(Protobuf_INCLUDE_DIR ${protobuf_SOURCE_DIR}/src/ CACHE STRING "Protobuf include directory.")
    set(PROTOBUF_LIBRARY libprotobuf)
    message("Done with setting up Protobuf for P4C.")
  endif()
endmacro(p4c_obtain_protobuf)
