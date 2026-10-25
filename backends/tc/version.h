/*
 * Copyright (C) 2023 Intel Corporation
 * SPDX-FileCopyrightText: 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_TC_VERSION_H_
#define BACKENDS_TC_VERSION_H_

#include <string>

/* Start version defines
** The build process will define these macros
** so we can have the same version number as the build process */

#ifndef MAJORVERSION
#define MAJORVERSION 1
#endif

#ifndef MINORVERSION
#define MINORVERSION 0
#endif

#ifndef BUILDVERSION
#define BUILDVERSION 0
#endif

#ifndef FIXVERSION
#define FIXVERSION 0
#endif
/* end version defines*/

std::string version_string();
unsigned int version_num();

#endif /* BACKENDS_TC_VERSION_H_ */
