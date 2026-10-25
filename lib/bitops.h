/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_BITOPS_H_
#define LIB_BITOPS_H_

#include <limits.h>

#include "bitvec.h"

namespace P4 {

static inline unsigned bitcount(unsigned v) {
#if defined(__GNUC__) || defined(__clang__)
    unsigned rv = __builtin_popcount(v);
#else
    unsigned rv = 0;
    while (v) {
        v &= v - 1;
        ++rv;
    }
#endif
    return rv;
}

static inline int floor_log2(unsigned v) {
    int rv = -1;
#if defined(__GNUC__) || defined(__clang__)
    if (v) rv = CHAR_BIT * sizeof(unsigned) - __builtin_clz(v) - 1;
#else
    while (v) {
        rv++;
        v >>= 1;
    }
#endif
    return rv;
}

static inline int ceil_log2(unsigned v) { return v ? floor_log2(v - 1) + 1 : -1; }

static inline unsigned bitmask2bytemask(const bitvec &a) {
    int max = a.max().index();
    if (max < 0) return 0;
    unsigned rv = 0;
    for (unsigned i = 0; i <= max / 8U; i++)
        if (a.getrange(i * 8, 8)) rv |= 1 << i;
    return rv;
}

}  // namespace P4

#endif /* LIB_BITOPS_H_ */
