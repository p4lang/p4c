/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* -*-C++-*- */

#ifndef LIB_NULL_H_
#define LIB_NULL_H_

#include "exceptions.h"  // for BUG macro

// Typical C contortions to transform something into a string
#define LIB_STRINGIFY(x) #x
#define LIB_TOSTRING(x) LIB_STRINGIFY(x)

#define CHECK_NULL(a)                                                              \
    do {                                                                           \
        if ((a) == nullptr) BUG(__FILE__ ":" LIB_TOSTRING(__LINE__) ": Null " #a); \
    } while (0)

#endif /* LIB_NULL_H_ */
