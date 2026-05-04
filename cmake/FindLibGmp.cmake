# Copyright 2017 Xilinx, Inc.
# SPDX-FileCopyrightText: 2017 Xilinx, Inc.
#
# SPDX-License-Identifier: Apache-2.0

# FindLibGmp
# -----------
#
# Try to find LibGmp the GNU Multiple Precision Arithmetic Library
#
# Once done this will define
#
# ::
#
#   LIBGMP_FOUND - System has LibGmp
#   LIBGMP_INCLUDE_DIR - The LibGmp include directory
#   LIBGMP_LIBRARIES - The libraries needed to use LibGmp
#   LIBGMP_DEFINITIONS - Compiler switches required for using LibGmp
find_package(PkgConfig QUIET)
find_package(Threads)
PKG_CHECK_MODULES(PC_LIBGMP QUIET gmp)
set(LIBGMP_DEFINITIONS ${PC_LIBGMP_CFLAGS_OTHER})


find_path(LIBGMP_INCLUDE_DIR NAMES gmp.h
    HINTS
    ${PC_LIBGMP_INCLUDEDIR}
    ${PC_LIBGMP_INCLUDE_DIRS}
    )

find_library(LIBGMP_LIBRARY NAMES gmp libgmp
    HINTS
    ${PC_LIBGMP_LIBDIR}
    ${PC_LIBGMP_LIBRARY_DIRS}
    )

find_library(LIBGMPXX_LIBRARY NAMES gmpxx libgmpxx
    HINTS
    ${PC_LIBGMP_LIBDIR}
    ${PC_LIBGMP_LIBRARY_DIRS}
    )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibGmp
    REQUIRED_VARS LIBGMPXX_LIBRARY LIBGMP_LIBRARY LIBGMP_INCLUDE_DIR
    )
mark_as_advanced(LIBGMP_INCLUDE_DIR LIBGMP_LIBRARY LIBGMPXX_LIBRARY)

if(LIBGMP_FOUND)
    set(LIBGMP_LIBRARIES ${LIBGMPXX_LIBRARY} ${LIBGMP_LIBRARY})
endif()
