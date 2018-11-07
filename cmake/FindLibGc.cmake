# Copyright 2013-present Barefoot Networks, Inc.
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

#.rst:
# FindLibGc
# -----------
#
# Try to find the LibGc A garbage collector for C and C++
# implementing the Boehm-Demers-Weiser Conservative Garbage Collector
#
# Once done this will define
#
# ::
#
#   LIBGC_FOUND - System has LibGc
#   LIBGC_INCLUDE_DIR - The LibGc include directory
#   LIBGC_LIBRARIES - The libraries needed to use LibGc
#   LIBGC_DEFINITIONS - Compiler switches required for using LibGc
#   LIBGC_VERSION_STRING - the version of LibGc found

# use pkg-config to get the directories and then use these values
# in the find_path() and find_library() calls
find_package(PkgConfig QUIET)
PKG_CHECK_MODULES(PC_LIBGC QUIET bdw-gc)
set(LIBGC_DEFINITIONS ${PC_LIBGC_CFLAGS_OTHER})

find_path(LIBGC_INCLUDE_DIR NAMES gc/gc.h
    HINTS
    ${PC_LIBGC_INCLUDEDIR}
    ${PC_LIBGC_INCLUDE_DIRS}
    PATH_SUFFIXES gc
    )

find_library(LIBGC_LIBRARY NAMES gc libgc
    HINTS
    ${PC_LIBGC_LIBDIR}
    ${PC_LIBGC_LIBRARY_DIRS}
    )

find_library(LIBGCCPP_LIBRARY NAMES gccpp libgccpp
    HINTS
    ${PC_LIBGC_LIBDIR}
    ${PC_LIBGC_LIBRARY_DIRS}
    )

if(PC_LIBGC_VERSION)
    set(LIBGC_VERSION_STRING ${PC_LIBGC_VERSION})
elseif(LIBGC_INCLUDE_DIR AND EXISTS "${LIBGC_INCLUDE_DIR}/gc/gc_version.h")
    file(STRINGS "${LIBGC_INCLUDE_DIR}/gc/gc_version.h" libgc_version_major
        REGEX "^#define[\t ]+GC_TMP_VERSION_MAJOR[\t ]+[0-9]+.*")
    file(STRINGS "${LIBGC_INCLUDE_DIR}/gc/gc_version.h" libgc_version_minor
        REGEX "^#define[\t ]+GC_TMP_VERSION_MINOR[\t ]+[0-9]+.*")
    file(STRINGS "${LIBGC_INCLUDE_DIR}/gc/gc_version.h" libgc_version_patch
        REGEX "^#define[\t ]+GC_TMP_VERSION_MICRO[\t ]+[0-9]+.*")

    # some earlier versions do not put the full version as a comment
    if (${libgc_version_patch})
      string(REGEX REPLACE "^#define[\t ]+GC_TMP_VERSION_MICRO[\t 0-9]+/\\*[\t ]+([0-9\\.]+)[\t ]+\\*/" "\\1"
        LIBGC_VERSION_STRING "${libgc_version_patch}")
    else()
      string(REGEX REPLACE "^#define[\t ]+GC_TMP_VERSION_MAJOR[\t ]+([0-9]+)" "\\1"
        libgc_version_major "${libgc_version_major}")
      string(REGEX REPLACE "^#define[\t ]+GC_TMP_VERSION_MINOR[\t ]+([0-9]+)" "\\1"
        libgc_version_minor "${libgc_version_minor}")
      set(LIBGC_VERSION_STRING "${libgc_version_major}.${libgc_version_minor}")
    endif()
    unset(libgc_version_major)
    unset(libgc_version_minor)
    unset(libgc_version_patch)
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibGc
    REQUIRED_VARS LIBGCCPP_LIBRARY LIBGC_LIBRARY LIBGC_INCLUDE_DIR
    VERSION_VAR LIBGC_VERSION_STRING)

mark_as_advanced(LIBGC_INCLUDE_DIR LIBGC_LIBRARY LIBGCCPP_LIBRARY)

if(LIBGC_FOUND)
    set(LIBGC_LIBRARIES ${LIBGC_LIBRARY} ${LIBGCCPP_LIBRARY})
endif()
