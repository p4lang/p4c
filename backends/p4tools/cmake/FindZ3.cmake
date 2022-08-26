find_package(PkgConfig)
pkg_check_modules(PC_Z3 QUIET Z3)

find_path(Z3_INCLUDE_DIR
  NAMES z3++.h
  HINTS ${PC_Z3_INCLUDEDIR} ${PC_Z3_INCLUDE_DIRS}
  PATH_SUFFIXES z3 Z3
)

find_library(Z3_LIBRARY
  NAMES libz3.a
  HINTS ${PC_Z3_LIBDIR} ${PC_Z3_LIBRARY_DIRS}
  #PATH_SUFFIXES z3 Z3
)

find_program(Z3_EXEC
  NAMES z3
  PATH_SUFFIXES z3 Z3
)

mark_as_advanced(Z3_FOUND Z3_INCLUDE_DIR Z3_LIBRARY Z3_EXEC)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Z3 DEFAULT_MSG
  Z3_INCLUDE_DIR
  Z3_LIBRARY
)

if(Z3_FOUND)

  execute_process(
    COMMAND ${Z3_EXEC} --version > ${Z3_VERSION_FILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    OUTPUT_VARIABLE Z3_VERSION
  )
  set(Z3_VERSION_REGEX "^Z3 version (.+)$")
  string(REGEX REPLACE ${Z3_VERSION_REGEX} "\\1" Z3_VERSION "${Z3_VERSION}")

endif()

message(STATUS "Z3 version: ${Z3_VERSION}")
message(STATUS "Z3 lib dir: ${Z3_LIBRARY}")
message(STATUS "Z3 include dir: ${Z3_INCLUDE_DIR}")

# create imported target z3::z3
if(Z3_FOUND AND NOT TARGET z3::z3)
  add_library(z3::z3 INTERFACE IMPORTED)
  set_target_properties(z3::z3 PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${Z3_INCLUDE_DIR}")
  set_target_properties(z3::z3 PROPERTIES
    INTERFACE_LINK_LIBRARIES "${Z3_LIBRARY}")
endif()

