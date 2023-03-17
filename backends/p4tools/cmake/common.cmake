# Common configuration across all P4Tools projects.

# Use ccache if available.
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  message(STATUS "Enabling ccache")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

# Helper for defining a p4tools executable target.
function(add_p4tools_executable target source)
  add_executable(${target} ${source} ${ARGN})
  target_include_directories(
    ${target}
    PUBLIC "${CMAKE_SOURCE_DIR}"
    PUBLIC "${CMAKE_BINARY_DIR}"
  )
  install(TARGETS ${target} RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})
endfunction(add_p4tools_executable)

# Helper for defining a p4tools library target.
function(add_p4tools_library target)
  add_library(${target} ${ARGN})
  target_include_directories(
    ${target}
    PUBLIC "${CMAKE_SOURCE_DIR}"
    PUBLIC "${CMAKE_BINARY_DIR}"
  )
endfunction(add_p4tools_library)

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

    # Print out download state while setting up Z3.
    set(FETCHCONTENT_QUIET_PREV ${FETCHCONTENT_QUIET})
    set(FETCHCONTENT_QUIET OFF)
    fetchcontent_declare(
      z3
      URL https://github.com/Z3Prover/z3/releases/download/z3-${P4TOOLS_Z3_VERSION}/z3-${P4TOOLS_Z3_VERSION}-x64-glibc-2.31.zip
      URL_HASH SHA256=9d0f70e61e82b321f35e6cad1343615d2dead6f2c54337a24293725de2900fb6
      USES_TERMINAL_DOWNLOAD TRUE
      GIT_PROGRESS TRUE
    )
    fetchcontent_makeavailable(z3)
    set(FETCHCONTENT_QUIET ${FETCHCONTENT_QUIET_PREV})
    message("Done with setting up Z3 for P4Tools.")

    # Other projects may also pull in Z3.
    # We have to make sure we only include our local version with P4Tools.
    set(P4TOOLS_Z3_LIB ${z3_SOURCE_DIR}/bin/libz3.a)
    set(P4TOOLS_Z3_INCLUDE_DIR ${z3_SOURCE_DIR}/include)
  endif()
endmacro(p4tools_obtain_z3)
