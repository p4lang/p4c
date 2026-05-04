/*
SPDX-FileCopyrightText: 2018 Barefoot Networks, Inc.

SPDX-License-Identifier: Apache-2.0
*/

#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_VERSION_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_VERSION_H_

/**
  Set the compiler version at build time.
  The build system defines P4C_VERSION as a full string as well as the
  following components: P4C_VERSION_MAJOR, P4C_VERSION_MINOR,
  P4C_VERSION_PATCH, P4C_VERSION_RC, and P4C_GIT_SHA.

  They can be used to construct a version string as follows:
  #define VERSION_STRING "@P4C_VERSION@"
  or
  #define VERSION_STRING "@P4C_VERSION_MAJOR@.@P4C_VERSION_MINOR@.@P4C_VERSION_PATCH@@P4C_VERSION_RC@"

  Or, since this is backend specific, feel free to define other numbering
  scheme.

  */

#define SMITH_VERSION_STRING "@P4C_VERSION@"


#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_VERSION_H_ */
