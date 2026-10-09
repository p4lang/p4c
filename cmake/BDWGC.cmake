# SPDX-FileCopyrightText: 2024 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

macro(p4c_obtain_bdwgc)
  p4c_dependency_compat(P4C_USE_PREINSTALLED_BDWGC LibGc)
  p4c_find_package(
    NAME LibGc
    VERSION 7.2.0
    GIT_REPOSITORY https://github.com/ivmai/bdwgc.git
    GIT_TAG 7f1503dbfe254e77678666a0e09b735add064b57 # 8.2.6
    FIND_PACKAGE_ARGUMENTS "MODULE ${P4C_LibGc_FIND_ARGUMENTS}"
    EXCLUDE_FROM_ALL YES
    SYSTEM YES
    OPTIONS
      "BUILD_SHARED_LIBS OFF"
      "enable_cplusplus ON"
      "install_headers OFF"
      "enable_docs ${ENABLE_DOCS}"
      "enable_large_config ON"
      "enable_redirect_malloc OFF"
      "enable_thread_local_alloc ON"
      "without_libatomic_ops OFF"
      "enable_disclaim ON"
      "enable_handle_fork ON"
  )
  set(LIBGC_LIBRARIES gc gctba cord)
  set(HAVE_LIBGC 1)

  if(NOT LibGc_SOURCE_DIR)
    option(ENABLE_MULTITHREAD "Use multithreading" OFF)
    find_package(LibGc 7.2.0 REQUIRED)
    # Some distros ship libgc/libcord without libgctba; only add it if present.
    find_library(LIBGC_GCTBA_LIBRARY NAMES gctba)
    if(NOT LIBGC_GCTBA_LIBRARY)
      message(STATUS "libgctba not found; using libgc/libcord only")
      set(LIBGC_LIBRARIES gc cord)
    endif()
    check_function_exists (GC_print_stats HAVE_GC_PRINT_STATS)
  elseif(LibGc_ADDED)
    # Bdwgc source code may trigger warnings which we need to ignore.
    target_compile_options(gc PRIVATE "-Wno-error" "-w")
    target_compile_options(gccpp PRIVATE "-Wno-error" "-w")
    target_compile_options(cord PRIVATE "-Wno-error" "-w")

    # Add some extra compile definitions which allow us to use our customizations.
    # SKIP_CPP_DEFINITIONS to enable static linking without producing duplicate symbol errors.
    # GC_USE_DLOPEN_WRAP needs to be enabled to handle threads correctly. This option is usually active with "enable_redirect_malloc" but we currently supply our own malloc overrides.
    target_compile_definitions(gc PUBLIC GC_USE_DLOPEN_WRAP SKIP_CPP_DEFINITIONS _GNU_SOURCE)
    target_compile_definitions(gccpp PUBLIC GC_USE_DLOPEN_WRAP SKIP_CPP_DEFINITIONS _GNU_SOURCE)

    # Set up temporary variable modifications for check_symbol_exists.
    set(CMAKE_REQUIRED_INCLUDES_PREV ${CMAKE_REQUIRED_INCLUDES})
    set(CMAKE_REQUIRED_LIBRARIES_PREV ${CMAKE_REQUIRED_LIBRARIES})
    set(CMAKE_REQUIRED_INCLUDES ${LibGc_SOURCE_DIR}/include)
    set(CMAKE_REQUIRED_LIBRARIES ${LIBGC_LIBRARIES})
    check_symbol_exists(GC_print_stats private/gc_priv.h HAVE_GC_PRINT_STATS)
    # Reset all temporary variable modifications.
    set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES_PREV})
    set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES_PREV})

    message(STATUS "Done with setting up BDWGC for P4C.")
  endif()
endmacro(p4c_obtain_bdwgc)
