/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_RESERVEDWORDS_H_
#define FRONTENDS_P4_RESERVEDWORDS_H_

#include <set>

#include "lib/cstring.h"

namespace P4 {

extern const std::set<cstring> reservedWords;

}  // namespace P4

#endif /* FRONTENDS_P4_RESERVEDWORDS_H_ */
