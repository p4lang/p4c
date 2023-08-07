macro(p4c_obtain_googletest)
  # Some P4C source files include gtest files.
  # We need this definition to guard against compilation errors.
  add_definitions(-DP4C_GTEST_ENABLED)
  # Print download state while setting up GTest.
  set(FETCHCONTENT_QUIET_PREV ${FETCHCONTENT_QUIET})
  set(FETCHCONTENT_QUIET OFF)
  set(INSTALL_GTEST OFF CACHE BOOL "Enable installation of googletest. (Projects embedding googletest may want to turn this OFF.")
  set(gtest_disable_pthreads ON CACHE BOOL "Disable uses of pthreads in gtest.")
  # Fetch and build the GTest dependency.
  FetchContent_Declare(
    gtest
    GIT_REPOSITORY https://github.com/google/googletest.git
    # https://github.com/google/googletest/releases/tag/v1.14.0
    GIT_TAG        f8d7d77c06936315286eb55f8de22cd23c188571
    GIT_PROGRESS TRUE
  )
  FetchContent_MakeAvailable(gtest)
  set(FETCHCONTENT_QUIET ${FETCHCONTENT_QUIET_PREV})
  message("Done with setting up GoogleTest for P4C.")

  include_directories(BEFORE SYSTEM ${gtest_SOURCE_DIR}/googletest/include)
  include_directories(BEFORE SYSTEM ${gtest_SOURCE_DIR}/googlemock/include)
  target_include_directories(gtest SYSTEM BEFORE PUBLIC ${gtest_SOURCE_DIR}/googletest/include)
  target_include_directories(gtest SYSTEM BEFORE PUBLIC ${gtest_SOURCE_DIR}/googlemock/include)
endmacro(p4c_obtain_googletest)
