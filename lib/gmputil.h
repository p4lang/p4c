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

#include "config.h"

#if HAVE_LIBGMP
#include <boost/multiprecision/gmp.hpp>
typedef boost::multiprecision::mpz_int big_int;
#else
#include <boost/multiprecision/cpp_int.hpp>
typedef boost::multiprecision::cpp_int big_int;
#endif

namespace Util {

// Useful functions for manipulating GMP values
// (arbitrary-precision values)

big_int ripBits(big_int &value, int bits);

struct BitRange {
    unsigned  lowIndex;
    unsigned  highIndex;
    big_int value;
};

// Find a consecutive scan of 1 bits at the "bottom"
BitRange findOnes(const big_int &value);

big_int cvtInt(const char *s, unsigned base);
big_int shift_left(const big_int &v, unsigned bits);
big_int shift_right(const big_int &v, unsigned bits);
// Convert a slice [m:l] into a mask
big_int maskFromSlice(unsigned m, unsigned l);
big_int mask(unsigned bits);

#if HAVE_LIBGMP
inline unsigned scan0(const boost::multiprecision::mpz_int &val, unsigned pos) {
    return mpz_scan0(val.backend().data(), pos); }
inline unsigned scan1(const boost::multiprecision::mpz_int &val, unsigned pos) {
    return mpz_scan1(val.backend().data(), pos); }
#else
inline unsigned scan0_positive(const boost::multiprecision::cpp_int &val, unsigned pos) {
    while (boost::multiprecision::bit_test(val, pos)) ++pos;
    return pos; }
inline unsigned scan1_positive(const boost::multiprecision::cpp_int &val, unsigned pos) {
    if (val == 0 || pos > boost::multiprecision::msb(val)) return ~0U;
    unsigned lsb = boost::multiprecision::lsb(val);
    if (lsb >= pos) return lsb;
    while (!boost::multiprecision::bit_test(val, pos)) ++pos;
    return pos; }
inline unsigned scan0(const boost::multiprecision::cpp_int &val, unsigned pos) {
    if (val < 0) return scan1_positive(-val - 1, pos);
    return scan0_positive(val, pos); }
inline unsigned scan1(const boost::multiprecision::cpp_int &val, unsigned pos) {
    if (val < 0) return scan0_positive(-val - 1, pos);
    return scan1_positive(val, pos); }
#endif


}  // namespace Util

static inline unsigned bitcount(big_int v) {
    if (v < 0) return ~0U;
    unsigned rv = 0;
    while (v != 0) { v &= v-1; ++rv; }
    return rv;
}

static inline int ffs(big_int v) {
    if (v <= 0) return -1;
    return boost::multiprecision::lsb(v);
}

static inline int floor_log2(big_int v) {
    int rv = -1;
    while (v > 0) { rv++; v /= 2; }
    return rv;
}

#endif /* _LIB_GMPUTIL_H_ */
