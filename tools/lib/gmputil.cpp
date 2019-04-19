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

#include <stdexcept>
#include "gmputil.h"

namespace Util {

mpz_class shift_left(const mpz_class &v, unsigned bits) {
    mpz_class exp;
    mpz_class two(2);
    mpz_pow_ui(exp.get_mpz_t(), two.get_mpz_t(), bits);
    return v * exp;
}
mpz_class shift_right(const mpz_class &v, unsigned bits) {
    mpz_class result;
    mpz_fdiv_q_2exp(result.get_mpz_t(), v.get_mpz_t(), bits);
    return result;
}

// Returns last 'bits' of value
// value is shifted right by this many bits
mpz_class ripBits(mpz_class &value, int bits) {
    mpz_class last;
    mpz_class rest;

    mpz_fdiv_r_2exp(last.get_mpz_t(), value.get_mpz_t(), bits);
    mpz_fdiv_q_2exp(rest.get_mpz_t(), value.get_mpz_t(), bits);
    value = rest;
    return last;
}

mpz_class mask(unsigned bits) {
    mpz_class one = 1;
    mpz_class result = shift_left(one, bits);
    return result - 1;
}

mpz_class maskFromSlice(unsigned m, unsigned l) {
    if (m < l)
        throw std::logic_error("wrong argument order in maskFromSlice");
    mpz_class result = mask(m+1) - mask(l);
    return result;
}

/// Find a consecutive scan of 1 bits
BitRange findOnes(const mpz_class &value) {
    BitRange result;
    if (value != 0) {
        result.lowIndex = mpz_scan1(value.get_mpz_t(), 0);
        result.highIndex = mpz_scan0(value.get_mpz_t(), result.lowIndex) - 1;
        result.value = maskFromSlice(result.highIndex, result.lowIndex);
    } else {
        result.lowIndex = -1;
        result.highIndex = -1;
        result.value = 0;
    }
    return result;
}

mpz_class cvtInt(const char *s, unsigned base) {
    mpz_class rv;

    while (*s) {
        switch (*s) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            rv *= base;
            rv += *s - '0';
            break;
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
            rv *= base;
            rv += *s - 'a' + 10;
            break;
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
            rv *= base;
            rv += *s - 'A' + 10;
            break;
        case '_':
            break;
        default:
            throw std::logic_error(std::string("Unexpected character ") + *s);
        }
        s++;
    }
    return rv;
}

}  // namespace Util
