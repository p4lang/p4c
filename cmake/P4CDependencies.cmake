# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

# Preserve pinned source builds by default, while honoring CPM's environment
# override and making the setting visible in cmake-gui and ccmake.
if(DEFINED ENV{CPM_DOWNLOAD_ALL})
  set(P4C_CPM_DOWNLOAD_DEFAULT "$ENV{CPM_DOWNLOAD_ALL}")
else()
  set(P4C_CPM_DOWNLOAD_DEFAULT ON)
endif()
option(CPM_DOWNLOAD_ALL "Build CPM dependencies from source by default" ${P4C_CPM_DOWNLOAD_DEFAULT})
unset(P4C_CPM_DOWNLOAD_DEFAULT)

# CPMFindPackage searches installed packages before checking CPM's registry or
# source overrides. Honor a parent's existing package (including siblings) and
# explicit source checkouts first, using CPM's own registration/version checks.
macro(p4c_find_package)
  cmake_parse_arguments(P4C_CPM_ARGS "" "NAME" "" ${ARGN})
  set(${P4C_CPM_ARGS_NAME}_ADDED NO)
  set(${P4C_CPM_ARGS_NAME}_SOURCE_DIR "")
  set(${P4C_CPM_ARGS_NAME}_BINARY_DIR "")
  if("${P4C_CPM_ARGS_NAME}" IN_LIST CPM_PACKAGES OR
     NOT "${CPM_${P4C_CPM_ARGS_NAME}_SOURCE}" STREQUAL "")
    CPMAddPackage(${ARGN})
  else()
    CPMFindPackage(${ARGN})
  endif()
  if(${P4C_CPM_ARGS_NAME}_SOURCE_DIR AND
     NOT IS_DIRECTORY "${${P4C_CPM_ARGS_NAME}_SOURCE_DIR}")
    message(FATAL_ERROR
      "CPM did not populate ${P4C_CPM_ARGS_NAME}: ${${P4C_CPM_ARGS_NAME}_SOURCE_DIR}. "
      "If FETCHCONTENT_FULLY_DISCONNECTED is ON, turn it OFF for the initial fetch, "
      "or set CPM_${P4C_CPM_ARGS_NAME}_SOURCE to an existing checkout.")
  endif()
endmacro()

macro(p4c_dependency_compat legacy package)
  set(P4C_${package}_FIND_ARGUMENTS "")
  if(DEFINED ${legacy})
    if(${legacy})
      set(P4C_CPM_DOWNLOAD OFF)
    else()
      set(P4C_CPM_DOWNLOAD ON)
    endif()
    get_property(P4C_CPM_COMPAT_WARNED GLOBAL PROPERTY P4C_DEPRECATED_${legacy} SET)
    if(NOT P4C_CPM_COMPAT_WARNED)
      message(DEPRECATION
        "${legacy}=${${legacy}} is deprecated; use -DCPM_DOWNLOAD_${package}=${P4C_CPM_DOWNLOAD}. "
        "CPM permits source fallback when OFF; use CPM_LOCAL_PACKAGES_ONLY=ON to forbid downloads.")
      set_property(GLOBAL PROPERTY P4C_DEPRECATED_${legacy} TRUE)
    endif()
    if(NOT DEFINED CPM_DOWNLOAD_${package} AND NOT DEFINED ENV{CPM_DOWNLOAD_${package}})
      set(CPM_DOWNLOAD_${package} ${P4C_CPM_DOWNLOAD})
      if(${legacy})
        # Legacy ON required an installed package; do not silently fetch one.
        set(P4C_${package}_FIND_ARGUMENTS REQUIRED)
      endif()
    elseif(NOT P4C_CPM_COMPAT_WARNED)
      message(STATUS "Ignoring ${legacy}: CPM_DOWNLOAD_${package} was explicitly supplied.")
    endif()
    unset(P4C_CPM_DOWNLOAD)
  endif()
endmacro()
