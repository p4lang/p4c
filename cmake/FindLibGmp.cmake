
find_path(LibGmp_INCLUDE_DIR NAMES gmp.h)
find_library(LibGmp_LIBRARY NAMES gmp libgmp)
find_library(LibGmpxx_LIBRARY NAMES gmpxx libgmpxx)

include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
find_package_handle_standard_args(LibGmp
    REQUIRED_VARS LibGmp_INCLUDE_DIR LibGmp_LIBRARY LibGmpxx_LIBRARY
    )

set(LibGmp_FOUND ${LIBGMP_FOUND})
unset(LIBGMP_FOUND)

if(LibGmp_FOUND)
    set(LibGmp_INCLUDE_DIRS ${LibGmp_INCLUDE_DIR})
    set(LibGmp_LIBRARIES    ${LibGmp_LIBRARY};${LibGmpxx_LIBRARY})
endif()
