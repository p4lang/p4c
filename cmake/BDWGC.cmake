macro(p4c_obtain_bdwgc)
  include(CheckSymbolExists)
  # Turn on C++ support.
  set(enable_cplusplus ON CACHE BOOL "C++ support")
  set(install_headers ON CACHE BOOL "Install header and pkg-config metadata files.")
  set(enable_docs ${ENABLE_DOCS} CACHE BOOL "Build and install documentation.")
  # We may have to compile very large programs which allocate a lot on the heap.
  set(enable_large_config ON CACHE BOOL "Optimize for large heap or root set.")
  set(without_libatomic_ops OFF CACHE BOOL "Use atomic_ops.h in libatomic_ops/src")

  # Redirect all malloc calls in the program.
  set(enable_redirect_malloc ON CACHE BOOL "Redirect malloc and friends to GC routines.")
  add_definitions("-DREDIRECT_REALLOC=GC_REALLOC")
  # Try to enable thread-local-storage for better performance.
  set(enable_thread_local_alloc ON CACHE BOOL "Turn on thread-local allocation optimization")
  add_definitions("-DUSE_COMPILER_TLS")
  add_definitions("-DNO_PROC_FOR_LIBRARIES")

  fetchcontent_declare(
    bdwgc
    GIT_REPOSITORY https://github.com/ivmai/bdwgc.git
    GIT_TAG e340b2e869e02718de9c9d7fa440ef4b35785388 # 8.2.6
    GIT_PROGRESS TRUE
    PATCH_COMMAND
      git apply ${CMAKE_SOURCE_DIR}/tools/silence_gc_warnings.patch || git apply
      ${CMAKE_SOURCE_DIR}/tools/silence_gc_warnings.patch -R --check && echo
      "Patch does not apply because the patch was already applied."
  )
  fetchcontent_makeavailable(bdwgc)

  set(LIBGC_LIBRARIES gc gccpp cord)
  include_directories(SYSTEM ${gc_SOURCE_DIR}/include)
  # Now, enable it to be used in the code.
  set(HAVE_LIBGC 1)
  check_symbol_exists(GC_print_stats "gc.h" HAVE_GC_PRINT_STATS)
endmacro(p4c_obtain_bdwgc)
