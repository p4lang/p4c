# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

macro(p4c_obtain_multiprecision)
  p4c_dependency_compat(P4C_USE_PREINSTALLED_MULTIPRECISION multiprecision)
  set(P4C_MULTIPRECISION_VERSION "1.90.0")
  # Download only: standalone mode needs just the headers, not Boost's CMake targets.
  p4c_find_package(
    NAME multiprecision
    URL https://github.com/boostorg/multiprecision/archive/refs/tags/boost-${P4C_MULTIPRECISION_VERSION}.tar.gz
    URL_HASH SHA256=b28a20f95712e596b3eb8aa4ae2264363016a609a63ebc3d84ee8cb49831de67
    FIND_PACKAGE_ARGUMENTS "MODULE ${P4C_multiprecision_FIND_ARGUMENTS}"
    DOWNLOAD_ONLY YES
  )
  if(multiprecision_SOURCE_DIR)
    # Always pair fetched Multiprecision with the same release of Boost.Config.
    CPMAddPackage(
      NAME boost_config
      URL https://github.com/boostorg/config/archive/refs/tags/boost-${P4C_MULTIPRECISION_VERSION}.tar.gz
      URL_HASH SHA256=1390bd79fbf270e40cfe5ac6b899f4a9c4e47139b6e17b4c1f7f78822adaa9fc
      DOWNLOAD_ONLY YES
      FORCE YES
    )
    set(P4C_MULTIPRECISION_INCLUDE_DIR ${multiprecision_SOURCE_DIR}/include)
    set(P4C_BOOST_CONFIG_INCLUDE_DIR ${boost_config_SOURCE_DIR}/include)
    set(P4C_MULTIPRECISION_STANDALONE ON)
    add_compile_definitions(BOOST_MP_STANDALONE)
  else()
    find_package(multiprecision REQUIRED)
    set(P4C_MULTIPRECISION_STANDALONE OFF)
  endif()
  include_directories(SYSTEM ${P4C_MULTIPRECISION_INCLUDE_DIR} ${P4C_BOOST_CONFIG_INCLUDE_DIR})
  message(STATUS "Done with setting up Multiprecision for P4C.")
endmacro()
