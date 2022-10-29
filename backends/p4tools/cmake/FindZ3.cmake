find_package(PkgConfig)
pkg_check_modules(PC_Z3 QUIET Z3)

find_path(
  Z3_INCLUDE_DIR
  NAMES z3++.h
  HINTS ${PC_Z3_INCLUDEDIR} ${PC_Z3_INCLUDE_DIRS}
  PATH_SUFFIXES z3 Z3
)

find_library(
  Z3_LIBRARY
  NAMES z3
  HINTS ${PC_Z3_LIBDIR} ${PC_Z3_LIBRARY_DIRS}
)

find_program(
  Z3_EXEC
  NAMES z3
  PATH_SUFFIXES z3 Z3
)

mark_as_advanced(Z3_FOUND Z3_INCLUDE_DIR Z3_LIBRARY Z3_EXEC)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
  Z3 DEFAULT_MSG
  Z3_INCLUDE_DIR
  Z3_LIBRARY
)

if(Z3_FOUND)
  message(STATUS "Found Z3 includes at ${Z3_INCLUDE_DIR}.")
  file(READ "${Z3_INCLUDE_DIR}/z3_version.h" Z3_VERSION_FILE)

  # Major
  string(REGEX MATCH "Z3_MAJOR_VERSION[ ]+([0-9]*)" _ ${Z3_VERSION_FILE})
  set(Z3_VERSION_MAJOR ${CMAKE_MATCH_1})
  # Minor
  string(REGEX MATCH "Z3_MINOR_VERSION[ ]+([0-9]*)" _ ${Z3_VERSION_FILE})
  set(Z3_VERSION_MINOR ${CMAKE_MATCH_1})
  # Build
  string(REGEX MATCH "Z3_BUILD_NUMBER[ ]+([0-9]*)" _ ${Z3_VERSION_FILE})
  set(Z3_VERSION_PATCH ${CMAKE_MATCH_1})
  # Tweak
  string(REGEX MATCH "Z3_REVISION_NUMBER[ ]+([0-9]*)" _ ${Z3_VERSION_FILE})
  set(Z3_VERSION_TWEAK ${CMAKE_MATCH_1})

  set(Z3_VERSION_STRING "${Z3_VERSION_MAJOR}.${Z3_VERSION_MINOR}.${Z3_VERSION_PATCH}.${Z3_VERSION_TWEAK}")
endif()

message(STATUS "Z3 version: ${Z3_VERSION_STRING}")
message(STATUS "Z3 lib dir: ${Z3_LIBRARY}")
message(STATUS "Z3 include dir: ${Z3_INCLUDE_DIR}")

# create imported target z3::z3
if(Z3_FOUND AND NOT TARGET z3::z3)
  add_library(z3::z3 INTERFACE IMPORTED)
  set_target_properties(
    z3::z3 PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${Z3_INCLUDE_DIR}"
  )
  set_target_properties(
    z3::z3 PROPERTIES
    INTERFACE_LINK_LIBRARIES "${Z3_LIBRARY}"
  )
endif()
