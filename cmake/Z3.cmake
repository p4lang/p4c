# SPDX-FileCopyrightText: 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

macro(obtain_z3)
  p4c_dependency_compat(USE_PREINSTALLED_Z3 Z3)
  set(P4C_Z3_VERSION 4.13.3)
  # Share the patched sources across checkouts, but invalidate them when the
  # patch changes. A cache key based only on the patch's path would miss edits.
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
    # Avoid an uninstall target clash and malloc_usable_size calls incompatible with GC.
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
    # We need a fairly recent version of Z3.
    set(Z3_MIN_VERSION "4.8.14")
    # Anything above 4.12+ does not work with libGC and causes crashes. The reason is
    # that malloc_usable_size() does not seem to be supported in libgc.
    # https://github.com/Z3Prover/z3/blob/b0fef6429fb29a33eb14adf9a61353b59f0f7fd0/src/util/memory_manager.cpp#L319
    # Without being able to patch Z3 we have to limit the maximum version.
    set(Z3_MAX_VERSION_EXCL "4.12")
    find_package(Z3 ${Z3_MIN_VERSION} REQUIRED)

    if(NOT DEFINED Z3_VERSION_STRING OR ${Z3_VERSION_STRING} VERSION_LESS ${Z3_MIN_VERSION})
      message(FATAL_ERROR "The minimum required Z3 version is ${Z3_MIN_VERSION}. Has ${Z3_VERSION_STRING}.")
    endif()
    if(${Z3_VERSION_STRING} VERSION_GREATER_EQUAL ${Z3_MAX_VERSION_EXCL})
      message(FATAL_ERROR "The Z3 version has to be lower than ${Z3_MAX_VERSION_EXCL} (the latter currently does no work with libGC). Has ${Z3_VERSION_STRING}.")
    endif()
    # Set variables for later consumption.
    set(Z3_LIB z3::z3)
  else()
    # Suppress warnings for all Z3 targets.
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

    # Other projects may also pull in Z3.
    # We have to make sure we only include our local version.
    set(Z3_LIB libz3)
    set(Z3_INCLUDE_DIR ${Z3_SOURCE_DIR}/src/api ${Z3_SOURCE_DIR}/src/api/c++)
  endif()

  message(STATUS "Done with setting up Z3.")
endmacro(obtain_z3)
