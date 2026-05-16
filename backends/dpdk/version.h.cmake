/*
SPDX-FileCopyrightText: 2022 Intel Corp.

SPDX-License-Identifier: Apache-2.0
*/

#ifndef _BACKEND_DPDK_VERSION_H
#define _BACKEND_DPDK_VERSION_H

/**
  Set the compiler version at build time.
  The build system defines P4C_VERSION as a full string as well as the
  following components: P4C_VERSION_MAJOR, P4C_VERSION_MINOR,
  P4C_VERSION_PATCH, P4C_VERSION_RC, P4C_GIT_SHA and CMAKE_BUILD_TYPE.

  They can be used to construct a version string as follows:
  #define VERSION_STRING "@P4C_VERSION@"
  or
  #define VERSION_STRING "@P4C_VERSION_MAJOR@.@P4C_VERSION_MINOR@.@P4C_VERSION_PATCH@@P4C_VERSION_RC@"

  Or, since this is backend specific, feel free to define other numbering
  scheme.

  */

  #define DPDK_VERSION_STRING "0.1 (SHA: @P4C_GIT_SHA@ BUILD: @CMAKE_BUILD_TYPE@)"

  #endif  // _BACKEND_DPDK_VERSION_H
