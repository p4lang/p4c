/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef _LIB_GMPUTIL_H_
#define _LIB_GMPUTIL_H_

#include <cstddef>  // needed because of a bug in gcc-4.9/libgmp
#include <gmpxx.h>  // NOLINT: cstddef HAS to come first

namespace Util {

// Useful functions for manipulating GMP values
// (arbitrary-precision values)

mpz_class ripBits(mpz_class &value, int bits);

struct BitRange {
    unsigned  lowIndex;
    unsigned  highIndex;
    mpz_class value;
};

// Find a consecutive scan of 1 bits at the "bottom"
BitRange findOnes(const mpz_class &value);

mpz_class cvtInt(const char *s, unsigned base);
mpz_class shift_left(const mpz_class &v, unsigned bits);
mpz_class shift_right(const mpz_class &v, unsigned bits);
// Convert a slice [m:l] into a mask
mpz_class maskFromSlice(unsigned m, unsigned l);
mpz_class mask(unsigned bits);
}  // namespace Util


#include "exceptions.h"
static inline unsigned bitcount(mpz_class value) {
    mpz_class v = value;
    if (sgn(v) < 0)
        BUG("bitcount of negative number %1%", value);
    unsigned rv = 0;
    while (v != 0) { v &= v-1; ++rv; }
    return rv;
}

static inline int ffs(mpz_class v) {
    if (v == 0) return -1;
    return mpz_scan1(v.get_mpz_t(), 0);
}

static inline int floor_log2(mpz_class v) {
    int rv = -1;
    while (v > 0) { rv++; v /= 2; }
    return rv;
}

#endif /* _LIB_GMPUTIL_H_ */
