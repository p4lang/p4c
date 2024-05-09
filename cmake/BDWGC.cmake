macro(p4c_obtain_bdwgc)
  option(
    P4C_USE_PREINSTALLED_BDWGC
    "Look for a preinstalled version of BDWGC in the system instead of installing a prebuilt binary using FetchContent."
    OFF
  )
  set(LIBGC_LIBRARIES gc gccpp cord)
  set(HAVE_LIBGC 1)

  if(P4C_USE_PREINSTALLED_BDWGC)
    option(ENABLE_MULTITHREAD "Use multithreading" OFF)
    find_package(LibGc 7.2.0 REQUIRED)
    check_function_exists (GC_print_stats HAVE_GC_PRINT_STATS)
  else()
    # Print out download state while setting up BDWGC.
    set(FETCHCONTENT_QUIET_PREV ${FETCHCONTENT_QUIET})
    set(BUILD_SHARED_LIBS_PREV ${BUILD_SHARED_LIBS})
    set(FETCHCONTENT_QUIET OFF)

    # BDWGC options.
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "")
    # Turn on C++ support.
    set(enable_cplusplus ON CACHE BOOL "C++ support")
    set(install_headers OFF CACHE BOOL "Install header and pkg-config metadata files.")
    set(enable_docs ${ENABLE_DOCS} CACHE BOOL "Build and install documentation.")
    # We may have to compile very large programs which allocate a lot on the heap.
    set(enable_large_config ON CACHE BOOL "Optimize for large heap or root set.")
    # Redirect all malloc calls in the program.
    set(enable_redirect_malloc OFF CACHE BOOL "Redirect malloc and friends to GC routines.")
    # Try to enable thread-local-storage for better performance.
    set(enable_thread_local_alloc ON CACHE BOOL "Turn on thread-local allocation optimization")
    # Other BDWGC options to avoid crashes.
    set(without_libatomic_ops OFF CACHE BOOL "Use atomic_ops.h in libatomic_ops/src")
    set(enable_disclaim ON CACHE BOOL "Support alternative finalization interface")
    set(enable_handle_fork ON CACHE BOOL "Attempt to ensure a usable collector after fork()")

    fetchcontent_declare(
      bdwgc
      GIT_REPOSITORY https://github.com/ivmai/bdwgc.git
      GIT_TAG e340b2e869e02718de9c9d7fa440ef4b35785388 # 8.2.6
      GIT_PROGRESS TRUE
      PATCH_COMMAND
        git apply ${P4C_SOURCE_DIR}/cmake/bdwgc.patch || git apply
        ${P4C_SOURCE_DIR}/cmake/bdwgc.patch -R --check && echo
        "Patch does not apply because the patch was already applied."
    )
    fetchcontent_makeavailable_but_exclude_install(bdwgc)

    # Bdwgc source code may trigger warnings which we need to ignore.
    target_compile_options(gc PRIVATE "-Wno-error" "-w")
    target_compile_options(gccpp PRIVATE "-Wno-error" "-w")
    target_compile_options(cord PRIVATE "-Wno-error" "-w")

    # Add some extra compile definitions which allow us to use our customizations.
    # SKIP_CPP_DEFINITIONS to enable static linking without producing duplicate symbol errors.
    # GC_USE_DLOPEN_WRAP needs to be enabled to handle threads correctly. This option is usually active with "enable_redirect_malloc" but we currently supply our own malloc overrides.
    target_compile_definitions(gc PUBLIC GC_USE_DLOPEN_WRAP SKIP_CPP_DEFINITIONS)
    target_compile_definitions(gccpp PUBLIC GC_USE_DLOPEN_WRAP SKIP_CPP_DEFINITIONS)

    # Set up temporary variable modifications for check_symbol_exists.
    set(CMAKE_REQUIRED_INCLUDES_PREV ${CMAKE_REQUIRED_INCLUDES})
    set(CMAKE_REQUIRED_LIBRARIES_PREV ${CMAKE_REQUIRED_LIBRARIES})
    set(CMAKE_REQUIRED_INCLUDES ${gc_SOURCE_DIR}/include)
    set(CMAKE_REQUIRED_LIBRARIES ${LIBGC_LIBRARIES})
    check_symbol_exists(GC_print_stats private/gc_priv.h HAVE_GC_PRINT_STATS)
    # Reset all temporary variable modifications.
    set(FETCHCONTENT_QUIET ${FETCHCONTENT_QUIET_PREV})
    set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES_PREV})
    set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES_PREV})
    set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_PREV} CACHE BOOL "")

    message(STATUS "Done with setting up BDWGC for P4C.")
  endif()
endmacro(p4c_obtain_bdwgc)
