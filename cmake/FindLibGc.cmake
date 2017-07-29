
find_path(LibGc_INCLUDE_DIR NAMES gmp.h)
find_library(LibGc_LIBRARY NAMES gmp libgmp)

include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
find_package_handle_standard_args(LibGc
    REQUIRED_VARS LibGc_INCLUDE_DIR LibGc_LIBRARY
    )

set(LibGc_FOUND ${LIBGMP_FOUND})
unset(LIBGMP_FOUND)

if(LibGc_FOUND)
    set(LibGc_INCLUDE_DIRS ${LibGc_INCLUDE_DIR})
    set(LibGc_LIBRARIES    ${LibGc_LIBRARY})
endif()
