# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

# P4C builds pinned dependency sources by default. An existing CMake setting
# or CPM_DOWNLOAD_ALL environment variable can override this default.
if(DEFINED ENV{CPM_DOWNLOAD_ALL})
  set(P4C_CPM_DOWNLOAD_DEFAULT "$ENV{CPM_DOWNLOAD_ALL}")
else()
  set(P4C_CPM_DOWNLOAD_DEFAULT ON)
endif()
option(CPM_DOWNLOAD_ALL "Build CPM dependencies from source by default" ${P4C_CPM_DOWNLOAD_DEFAULT})
unset(P4C_CPM_DOWNLOAD_DEFAULT)

# Accept the same arguments as CPMFindPackage and expose CPM's result variables:
# <name>_SOURCE_DIR, <name>_BINARY_DIR, and <name>_ADDED. A source directory can
# belong to a package that the parent already added. P4C changes target settings
# only when _ADDED is true.
macro(p4c_find_package)
  cmake_parse_arguments(P4C_CPM_ARGS "" "NAME" "" ${ARGN})
  # Installed packages may leave these unset. Clear any values from an earlier call.
  set(${P4C_CPM_ARGS_NAME}_ADDED NO)
  set(${P4C_CPM_ARGS_NAME}_SOURCE_DIR "")
  set(${P4C_CPM_ARGS_NAME}_BINARY_DIR "")
  # CPMFindPackage can search installed packages before checking the registry
  # or a source override. Use CPMAddPackage for those cases so that an existing
  # package or explicit checkout takes precedence over installed packages.
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

# Translate an explicitly supplied USE_PREINSTALLED option during migration to
# CPM. Callers append P4C_<package>_FIND_ARGUMENTS to FIND_PACKAGE_ARGUMENTS so
# that legacy ON still requires an installed package when no CPM override is set.
macro(p4c_dependency_compat legacy_option package_name)
  set(P4C_${package_name}_FIND_ARGUMENTS "")
  if(DEFINED ${legacy_option})
    # USE_PREINSTALLED and CPM_DOWNLOAD have opposite boolean meanings.
    if(${legacy_option})
      set(P4C_CPM_LEGACY_DOWNLOAD OFF)
    else()
      set(P4C_CPM_LEGACY_DOWNLOAD ON)
    endif()

    # A dependency can be requested by several backends. Warn once per configure.
    get_property(P4C_CPM_DEPRECATION_REPORTED GLOBAL
      PROPERTY P4C_DEPRECATED_${legacy_option} SET)
    if(NOT P4C_CPM_DEPRECATION_REPORTED)
      message(DEPRECATION
        "${legacy_option}=${${legacy_option}} is deprecated; "
        "use -DCPM_DOWNLOAD_${package_name}=${P4C_CPM_LEGACY_DOWNLOAD}. "
        "CPM_DOWNLOAD_${package_name}=OFF looks for an installed package first. "
        "If no suitable package is found, CPM downloads and builds the pinned version. "
        "Set CPM_LOCAL_PACKAGES_ONLY=ON to require installed library dependencies. "
        "See docs/CPM.md for dependency configuration.")
      set_property(GLOBAL PROPERTY P4C_DEPRECATED_${legacy_option} TRUE)
    endif()

    # A package-specific CPM setting takes precedence over the legacy option.
    if(NOT DEFINED CPM_DOWNLOAD_${package_name} AND
       NOT DEFINED ENV{CPM_DOWNLOAD_${package_name}})
      set(CPM_DOWNLOAD_${package_name} ${P4C_CPM_LEGACY_DOWNLOAD})
      if(${legacy_option})
        set(P4C_${package_name}_FIND_ARGUMENTS REQUIRED)
      endif()
    elseif(NOT P4C_CPM_DEPRECATION_REPORTED)
      message(STATUS "Ignoring ${legacy_option}: CPM_DOWNLOAD_${package_name} was explicitly supplied.")
    endif()
    unset(P4C_CPM_LEGACY_DOWNLOAD)
  endif()
endmacro()
