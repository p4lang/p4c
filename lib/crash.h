/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_CRASH_H_
#define LIB_CRASH_H_

namespace P4 {

void setup_signals();
const char *addr2line(void *addr, const char *text);

}  // namespace P4

#endif /* LIB_CRASH_H_ */
