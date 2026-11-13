# SPDX-FileCopyrightText: 2024 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

macro(p4c_obtain_bdwgc)
  p4c_dependency_compat(P4C_USE_PREINSTALLED_BDWGC LibGc)
  p4c_find_package(
    NAME LibGc
    # Accept installed releases from 7.2.0 onward; source builds use the pinned revision.
    VERSION 7.2.0
    GIT_REPOSITORY https://github.com/ivmai/bdwgc.git
    GIT_TAG 7f1503dbfe254e77678666a0e09b735add064b57 # 8.2.6
    FIND_PACKAGE_ARGUMENTS "MODULE ${P4C_LibGc_FIND_ARGUMENTS}"
    EXCLUDE_FROM_ALL YES
    SYSTEM YES
    OPTIONS
      "BUILD_SHARED_LIBS OFF"
      # P4C uses the collector's C++ allocation interfaces.
      "enable_cplusplus ON"
      "install_headers OFF"
      "enable_docs ${ENABLE_DOCS}"
      # Compiling large P4 programs can require a large heap and root set.
      "enable_large_config ON"
      # lib/gc.cpp supplies P4C's malloc overrides.
      "enable_redirect_malloc OFF"
      # Thread-local allocation reduces contention in the collector.
      "enable_thread_local_alloc ON"
      # Use compiler atomic operations where BDWGC supports them.
      "without_libatomic_ops OFF"
      # Keep support for BDWGC's alternative finalization interface.
      "enable_disclaim ON"
      # Keep the collector usable in child processes after fork().
      "enable_handle_fork ON"
  )
  set(LIBGC_LIBRARIES gc gctba cord)
  set(HAVE_LIBGC 1)

  if(NOT LibGc_SOURCE_DIR)
    option(ENABLE_MULTITHREAD "Use multithreading" OFF)
    # Reload the library paths because CPM does not export find-module variables.
    find_package(LibGc 7.2.0 REQUIRED)
    check_function_exists(GC_print_stats HAVE_GC_PRINT_STATS)
  elseif(LibGc_ADDED)
    # Suppress dependency warnings only for targets that this call created.
    # Parent-provided targets retain the parent's settings.
    foreach(target gc gccpp cord)
      target_compile_options(${target} PRIVATE "-Wno-error" "-w")
    endforeach()

    # SKIP_CPP_DEFINITIONS avoids duplicate C++ allocation symbols when linking
    # statically with P4C's overrides. GC_USE_DLOPEN_WRAP preserves the collector's
    # handling of dynamic loading in threaded programs; BDWGC normally enables
    # it with enable_redirect_malloc, which P4C disables above.
    foreach(target gc gccpp)
      target_compile_definitions(${target} PUBLIC GC_USE_DLOPEN_WRAP SKIP_CPP_DEFINITIONS _GNU_SOURCE)
    endforeach()

    # GC_print_stats is declared in a private header in source builds.
    # Keep this probe's include paths and libraries out of subsequent checks.
    include(CMakePushCheckState)
    cmake_push_check_state()
    set(CMAKE_REQUIRED_INCLUDES ${LibGc_SOURCE_DIR}/include)
    set(CMAKE_REQUIRED_LIBRARIES ${LIBGC_LIBRARIES})
    check_symbol_exists(GC_print_stats private/gc_priv.h HAVE_GC_PRINT_STATS)
    cmake_pop_check_state()

    message(STATUS "Done with setting up BDWGC for P4C.")
  endif()
endmacro(p4c_obtain_bdwgc)
