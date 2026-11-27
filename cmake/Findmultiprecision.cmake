# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

# Installed Multiprecision must use the matching Boost.Config and dependencies.
find_path(P4C_MULTIPRECISION_INCLUDE_DIR boost/multiprecision/cpp_int.hpp
  HINTS ${Boost_INCLUDE_DIRS} ${BOOST_ROOT}/include)
find_path(P4C_BOOST_CONFIG_INCLUDE_DIR boost/config.hpp
  HINTS ${P4C_MULTIPRECISION_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ${BOOST_ROOT}/include)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(multiprecision REQUIRED_VARS
  P4C_MULTIPRECISION_INCLUDE_DIR P4C_BOOST_CONFIG_INCLUDE_DIR)
