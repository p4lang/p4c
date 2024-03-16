# Common configuration across all P4Tools projects.

# Use ccache if available.
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  message(STATUS "Enabling ccache")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

# Use libgc.
find_package(LibGc 7.2.0 REQUIRED)

# Helper for defining a p4tools executable target.
function(add_p4tools_executable target source)
  add_executable(${target} ${source} ${ARGN})
  install(TARGETS ${target} RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})
endfunction(add_p4tools_executable)

macro(p4tools_obtain_z3)
  option(TOOLS_USE_PREINSTALLED_Z3 "Look for a preinstalled version of Z3 instead of installing a prebuilt binary using FetchContent." OFF)

  if(TOOLS_USE_PREINSTALLED_Z3)
    # We need a fairly recent version of Z3.
    set(Z3_MIN_VERSION "4.8.14")
    # But 4.12+ is currently broken with libGC
    set(Z3_MAX_VERSION_EXCL "4.12")
    find_package(Z3 ${Z3_MIN_VERSION} REQUIRED)

    if(NOT DEFINED Z3_VERSION_STRING OR ${Z3_VERSION_STRING} VERSION_LESS ${Z3_MIN_VERSION})
      message(FATAL_ERROR "The minimum required Z3 version is ${Z3_MIN_VERSION}. Has ${Z3_VERSION_STRING}.")
    endif()
    if(${Z3_VERSION_STRING} VERSION_GREATER_EQUAL ${Z3_MAX_VERSION_EXCL})
      message(FATAL_ERROR "The Z3 version has to be lower than ${Z3_MAX_VERSION_EXCL} (the latter currently does no work with libGC). Has ${Z3_VERSION_STRING}.")
    endif()
    # Set variables for later consumption.
    set(P4TOOLS_Z3_LIB z3::z3)
    set(P4TOOLS_Z3_INCLUDE_DIR ${Z3_INCLUDE_DIR})
  else()
    # Pull in a specific version of Z3 and link against it.
    set(P4TOOLS_Z3_VERSION "4.11.2")
    message("Fetching Z3 version ${P4TOOLS_Z3_VERSION} for P4Tools...")

    # Determine platform to fetch pre-built Z3
    if (CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64")
      set(Z3_ARCH "x64")
      if (APPLE)
        set(Z3_PLATFORM_SUFFIX "osx-10.16")
        set(Z3_ZIP_HASH "a56b6c40d9251a963aabe1f15731dd88ad1cb801d0e7b16e45f8b232175e165c")
      elseif (UNIX)
        set(Z3_PLATFORM_SUFFIX "glibc-2.31")
        set(Z3_ZIP_HASH "9d0f70e61e82b321f35e6cad1343615d2dead6f2c54337a24293725de2900fb6")
      else()
        message(FATAL_ERROR "Unsupported system platform")
      endif()
    elseif(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
      set(Z3_ARCH "arm64")
      if (APPLE)
        set(Z3_PLATFORM_SUFFIX "osx-11.0")
        set(Z3_ZIP_HASH "c021f381fa3169b1f7fb3b4fae81a1d1caf0dd8aa4aa773f4ab9d5e28c6657a4")
      else()
        message(FATAL_ERROR "Unsupported system platform")
      endif()
    else()
      message(FATAL_ERROR "Unsupported system processor")
    endif()

    # Print out download state while setting up Z3.
    set(FETCHCONTENT_QUIET_PREV ${FETCHCONTENT_QUIET})
    set(FETCHCONTENT_QUIET OFF)
    fetchcontent_declare(
      z3
      URL https://github.com/Z3Prover/z3/releases/download/z3-${P4TOOLS_Z3_VERSION}/z3-${P4TOOLS_Z3_VERSION}-${Z3_ARCH}-${Z3_PLATFORM_SUFFIX}.zip
      URL_HASH SHA256=${Z3_ZIP_HASH}
      USES_TERMINAL_DOWNLOAD TRUE
      GIT_PROGRESS TRUE
    )
    fetchcontent_makeavailable(z3)
    set(FETCHCONTENT_QUIET ${FETCHCONTENT_QUIET_PREV})
    message("Done with setting up Z3 for P4Tools.")

    # Other projects may also pull in Z3.
    # We have to make sure we only include our local version with P4Tools.
    set(P4TOOLS_Z3_LIB ${z3_SOURCE_DIR}/bin/libz3${CMAKE_STATIC_LIBRARY_SUFFIX})
    set(P4TOOLS_Z3_INCLUDE_DIR ${z3_SOURCE_DIR}/include)
  endif()
endmacro(p4tools_obtain_z3)
