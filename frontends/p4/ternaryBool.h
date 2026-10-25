/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_TERNARYBOOL_H_
#define FRONTENDS_P4_TERNARYBOOL_H_

#include "lib/cstring.h"

namespace P4 {
enum class TernaryBool { Yes, No, Maybe };

cstring toString(const TernaryBool &c);

}  // namespace P4

#endif /* FRONTENDS_P4_TERNARYBOOL_H_ */
