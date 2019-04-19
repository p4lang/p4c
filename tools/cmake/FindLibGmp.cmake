# Copyright 2017 Xilinx, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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
