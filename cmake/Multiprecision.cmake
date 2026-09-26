# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

macro(p4c_obtain_multiprecision)
  option(
    P4C_USE_PREINSTALLED_MULTIPRECISION
    "Look for preinstalled Multiprecision and Boost.Config headers instead of fetching them using FetchContent."
    OFF
  )

  if(P4C_USE_PREINSTALLED_MULTIPRECISION)
    find_path(P4C_MULTIPRECISION_INCLUDE_DIR boost/multiprecision/cpp_int.hpp
      HINTS ${Boost_INCLUDE_DIRS} ${BOOST_ROOT}/include)
    find_path(P4C_BOOST_CONFIG_INCLUDE_DIR boost/config.hpp
      HINTS ${P4C_MULTIPRECISION_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ${BOOST_ROOT}/include)
    if(NOT P4C_MULTIPRECISION_INCLUDE_DIR OR NOT P4C_BOOST_CONFIG_INCLUDE_DIR)
      message(FATAL_ERROR "Boost.Multiprecision and Boost.Config headers are required")
    endif()
    if(NOT EXISTS "${P4C_MULTIPRECISION_INCLUDE_DIR}/boost/multiprecision/detail/standalone_config.hpp")
      message(FATAL_ERROR
        "Multiprecision headers with standalone support are required. Set P4C_USE_PREINSTALLED_MULTIPRECISION=OFF to fetch them, or set P4C_MULTIPRECISION_INCLUDE_DIR to a standalone installation.")
    endif()
  else()
    set(P4C_MULTIPRECISION_VERSION "1.90.0")
    message(STATUS "Fetching Multiprecision version ${P4C_MULTIPRECISION_VERSION} for P4C...")

    set(FETCHCONTENT_QUIET_PREV ${FETCHCONTENT_QUIET})
    set(FETCHCONTENT_QUIET OFF)
    set(BUILD_TESTING_PREV ${BUILD_TESTING})
    set(BUILD_TESTING OFF)
    set(BOOST_MP_STANDALONE ON)

    # Standalone Multiprecision needs only Boost.Config, not the full Boost distribution.
    FetchContent_Declare(
      boost_config
      URL https://github.com/boostorg/config/archive/refs/tags/boost-${P4C_MULTIPRECISION_VERSION}.tar.gz
      URL_HASH SHA256=1390bd79fbf270e40cfe5ac6b899f4a9c4e47139b6e17b4c1f7f78822adaa9fc
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_Declare(
      multiprecision
      URL https://github.com/boostorg/multiprecision/archive/refs/tags/boost-${P4C_MULTIPRECISION_VERSION}.tar.gz
      URL_HASH SHA256=b28a20f95712e596b3eb8aa4ae2264363016a609a63ebc3d84ee8cb49831de67
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    fetchcontent_makeavailable_but_exclude_install(boost_config)
    fetchcontent_makeavailable_but_exclude_install(multiprecision)

    set(P4C_MULTIPRECISION_INCLUDE_DIR ${multiprecision_SOURCE_DIR}/include)
    set(P4C_BOOST_CONFIG_INCLUDE_DIR ${boost_config_SOURCE_DIR}/include)
    set(BUILD_TESTING ${BUILD_TESTING_PREV})
    set(FETCHCONTENT_QUIET ${FETCHCONTENT_QUIET_PREV})
  endif()

  include_directories(SYSTEM ${P4C_MULTIPRECISION_INCLUDE_DIR} ${P4C_BOOST_CONFIG_INCLUDE_DIR})
  add_compile_definitions(BOOST_MP_STANDALONE)
  message(STATUS "Done with setting up Multiprecision for P4C.")
endmacro(p4c_obtain_multiprecision)
