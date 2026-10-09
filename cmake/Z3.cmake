# SPDX-FileCopyrightText: 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

macro(obtain_z3)
  p4c_dependency_compat(USE_PREINSTALLED_Z3 Z3)
  set(P4C_Z3_VERSION 4.13.3)
  # Include the patch contents in the cache key so that changing the patch
  # selects a new source directory, even when the Z3 version stays the same.
  file(SHA256 "${P4C_SOURCE_DIR}/cmake/z3.patch" P4C_Z3_PATCH_HASH)
  p4c_find_package(
    NAME Z3
    GIT_REPOSITORY https://github.com/Z3Prover/z3.git
    GIT_TAG z3-${P4C_Z3_VERSION}
    GIT_SHALLOW YES
    CUSTOM_CACHE_KEY ${P4C_Z3_VERSION}-${P4C_Z3_PATCH_HASH}
    FIND_PACKAGE_ARGUMENTS "MODULE ${P4C_Z3_FIND_ARGUMENTS}"
    EXCLUDE_FROM_ALL YES
    SYSTEM YES
    # CPM adds Z3 to the same CMake build, where target names must be unique.
    # The patch renames Z3's uninstall target to avoid a conflict with P4C's.
    # The reverse check accepts sources where the patch is already applied.
    PATCH_COMMAND
      git apply ${P4C_SOURCE_DIR}/cmake/z3.patch || git apply
      ${P4C_SOURCE_DIR}/cmake/z3.patch -R --check
    OPTIONS
      "CMAKE_UNITY_BUILD OFF"
      "Z3_BUILD_LIBZ3_SHARED OFF"
      "Z3_INCLUDE_GIT_HASH OFF"
      "Z3_INCLUDE_GIT_DESCRIBE OFF"
      "Z3_BUILD_TEST_EXECUTABLES OFF"
      "Z3_SAVE_CLANG_OPTIMIZATION_RECORDS OFF"
      "Z3_ENABLE_TRACING_FOR_NON_DEBUG OFF"
      "Z3_ENABLE_EXAMPLE_TARGETS OFF"
      "Z3_BUILD_DOTNET_BINDINGS OFF"
      "Z3_BUILD_PYTHON_BINDINGS OFF"
      "Z3_BUILD_JAVA_BINDINGS OFF"
      "Z3_BUILD_JULIA_BINDINGS OFF"
      "Z3_BUILD_DOCUMENTATION OFF"
      "Z3_API_LOG_SYNC OFF"
      "Z3_BUILD_EXECUTABLE OFF"
  )

  if(NOT Z3_SOURCE_DIR)
    set(Z3_MIN_VERSION "4.8.12")
    find_package(Z3 ${Z3_MIN_VERSION} REQUIRED)

    if(ENABLE_GC)
      include(CheckCXXSourceCompiles)
      include(CMakePushCheckState)
      cmake_push_check_state(RESET)
      check_cxx_source_compiles("
        #include <cstdlib>
        #if !defined(__GLIBC__) && !defined(__APPLE__) && !defined(__FreeBSD__)
        #error No GC allocation-size query override
        #endif
        int main() { return 0; }
      " Z3_HAS_GC_SIZE_QUERY)
      cmake_pop_check_state()
      # Z3 4.12 and newer query allocation sizes. With GC enabled, these
      # queries must use the GC allocator rather than the system allocator.
      # lib/gc.cpp handles this on glibc, macOS, and FreeBSD.
      if(NOT Z3_HAS_GC_SIZE_QUERY AND Z3_VERSION_STRING VERSION_GREATER_EQUAL "4.12")
        message(FATAL_ERROR "System Z3 ${Z3_VERSION_STRING} is not supported with GC on this platform. Disable ENABLE_GC.")
      endif()
    endif()
    set(Z3_LIB z3::z3)
  else()
    # Suppress warnings only for the Z3 targets that this call added.
    if(Z3_ADDED)
      get_all_targets(Z3_BUILD_TARGETS ${Z3_SOURCE_DIR})
      foreach(target ${Z3_BUILD_TARGETS})
        get_target_property(target_type ${target} TYPE)
        if(NOT target_type STREQUAL "INTERFACE_LIBRARY" AND NOT target_type STREQUAL "UTILITY")
          target_compile_options(${target} PRIVATE "-Wno-error" "-w")
          # Z3 needs its own headers first to avoid conflicts with installed headers.
          target_include_directories(${target} BEFORE PRIVATE ${Z3_SOURCE_DIR}/src)
        endif()
      endforeach()
    endif()

    # Use the selected source package's library and headers together.
    set(Z3_LIB libz3)
    set(Z3_INCLUDE_DIR ${Z3_SOURCE_DIR}/src/api ${Z3_SOURCE_DIR}/src/api/c++)
  endif()

  message(STATUS "Done with setting up Z3.")
endmacro(obtain_z3)
