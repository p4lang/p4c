# SPDX-FileCopyrightText: 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

macro(obtain_z3)
  option(USE_PREINSTALLED_Z3 "Look for a preinstalled version of Z3 instead of installing a prebuilt binary using FetchContent." OFF)

  if(USE_PREINSTALLED_Z3)
    # TEMPORARY: exercise Ubuntu 20.04's Z3 in CI before deciding whether to
    # lower the supported minimum. Drop this change with the validation commit.
    set(Z3_MIN_VERSION "4.8.7")
    find_package(Z3 ${Z3_MIN_VERSION} REQUIRED)

    if(ENABLE_GC)
      include(CheckCXXSourceCompiles)
      include(CMakePushCheckState)
      cmake_push_check_state(RESET)
      check_cxx_source_compiles("
        #include <cstdlib>
        #ifndef __GLIBC__
        #error Not using glibc
        #endif
        int main() { return 0; }
      " Z3_USES_GLIBC)
      cmake_pop_check_state()
      # Z3 4.12 and newer query allocation sizes. With GC enabled, these
      # queries must use the GC allocator rather than the system allocator.
      # lib/gc.cpp handles this on glibc. Other platforms still need the
      # bundled Z3 patch, which disables these queries.
      if(NOT Z3_USES_GLIBC AND Z3_VERSION_STRING VERSION_GREATER_EQUAL "4.12")
        message(FATAL_ERROR "System Z3 ${Z3_VERSION_STRING} is not supported with GC on this platform. Use bundled Z3 or disable ENABLE_GC.")
      endif()
    endif()
    # Set variables for later consumption.
    set(Z3_LIB z3::z3)
    set(Z3_INCLUDE_DIR ${Z3_INCLUDE_DIR})
  else()
    # Pull in a specific version of Z3 and link against it.
    set(Z3_VERSION "4.13.0")
    message(STATUS "Fetching Z3 version ${Z3_VERSION}...")

    # Unity builds do not work for Protobuf...
    set(CMAKE_UNITY_BUILD_PREV ${CMAKE_UNITY_BUILD})
    set(CMAKE_UNITY_BUILD OFF)
    # Print out download state while setting up Z3.
    set(FETCHCONTENT_QUIET_PREV ${FETCHCONTENT_QUIET})
    set(FETCHCONTENT_QUIET OFF)

    set(Z3_BUILD_LIBZ3_SHARED OFF CACHE BOOL "Build libz3 as a shared library if true, otherwise build a static library")
    set(Z3_INCLUDE_GIT_HASH OFF CACHE BOOL "Include git hash in version output")
    set(Z3_INCLUDE_GIT_DESCRIBE OFF CACHE BOOL "Include git describe output in version output")
    set(Z3_BUILD_TEST_EXECUTABLES OFF CACHE BOOL "Include git describe output in version output")
    set(Z3_SAVE_CLANG_OPTIMIZATION_RECORDS OFF CACHE BOOL "Include git describe output in version output")
    set(Z3_ENABLE_TRACING_FOR_NON_DEBUG OFF CACHE BOOL "Include git describe output in version output")
    set(Z3_ENABLE_EXAMPLE_TARGETS OFF CACHE BOOL "Include git describe output in version output")
    set(Z3_BUILD_DOTNET_BINDINGS OFF CACHE BOOL "Include git describe output in version output")
    set(Z3_BUILD_PYTHON_BINDINGS OFF CACHE BOOL "Include git describe output in version output")
    set(Z3_BUILD_JAVA_BINDINGS OFF CACHE BOOL "Include git describe output in version output")
    set(Z3_BUILD_JULIA_BINDINGS OFF CACHE BOOL "Include git describe output in version output")
    set(Z3_BUILD_DOCUMENTATION OFF CACHE BOOL "Include git describe output in version output")
    set(Z3_ALWAYS_BUILD_DOCS OFF CACHE BOOL "Include git describe output in version output")
    set(Z3_API_LOG_SYNC OFF CACHE BOOL "Include git describe output in version output")
    set(Z3_BUILD_EXECUTABLE OFF CACHE BOOL "Include git describe output in version output")
    fetchcontent_declare(
      z3
      GIT_REPOSITORY https://github.com/Z3Prover/z3.git
      GIT_TAG z3-4.13.3 # 4.13.0
      GIT_PROGRESS TRUE
      GIT_SHALLOW TRUE
      # We need to patch because the Z3 CMakeLists.txt also adds an uinstall target.
      # This leads to a namespace clash.
      PATCH_COMMAND
        git apply ${P4C_SOURCE_DIR}/cmake/z3.patch || git apply
        ${P4C_SOURCE_DIR}/cmake/z3.patch -R --check && echo
        "Patch does not apply because the patch was already applied."
    )
    fetchcontent_makeavailable_but_exclude_install(z3)

    # Suppress warnings for all Z3 targets.
    get_all_targets(Z3_BUILD_TARGETS ${z3_SOURCE_DIR})
    foreach(target ${Z3_BUILD_TARGETS})
        # Do not suppress warnings for Z3 library targets that are aliased.
        get_target_property(target_type ${target} TYPE)
        if (NOT ${target_type} STREQUAL "INTERFACE_LIBRARY" AND NOT ${target_type} STREQUAL "UTILITY")
          target_compile_options(${target} PRIVATE "-Wno-error" "-w")
          # Z3 does not add its own include directories for compilation, which can lead to conflicts.
          target_include_directories(${target} BEFORE PRIVATE ${z3_SOURCE_DIR}/src)
      endif()
    endforeach()

    # Reset temporary variable modifications.
    set(CMAKE_UNITY_BUILD ${CMAKE_UNITY_BUILD_PREV})
    set(FETCHCONTENT_QUIET ${FETCHCONTENT_QUIET_PREV})

    # Other projects may also pull in Z3.
    # We have to make sure we only include our local version.
    set(Z3_LIB libz3)
    set(Z3_INCLUDE_DIR ${z3_SOURCE_DIR}/src/api ${z3_SOURCE_DIR}/src/api/c++)
  endif()

  message(STATUS "Done with setting up Z3.")
endmacro(obtain_z3)
