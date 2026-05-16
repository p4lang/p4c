// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "big_int_util.h"

#include <stdexcept>

namespace P4::Util {

using namespace boost::multiprecision;

big_int shift_left(const big_int &v, unsigned bits) { return v << bits; }
big_int shift_right(const big_int &v, unsigned bits) { return v >> bits; }

// Returns last 'bits' of value
// value is shifted right by this many bits
big_int ripBits(big_int &value, int bits) {
    big_int rv = 1;
    rv <<= bits;
    rv -= 1;
    rv &= value;
    value >>= bits;
    return rv;
}

big_int mask(unsigned bits) {
    big_int one = 1;
    big_int result = shift_left(one, bits);
    return result - 1;
}

big_int maskFromSlice(unsigned m, unsigned l) {
    if (m < l) throw std::logic_error("wrong argument order in maskFromSlice");
    big_int result = mask(m + 1) - mask(l);
    return result;
}

/// Find a consecutive scan of 1 bits
BitRange findOnes(const big_int &value) {
    BitRange result;
    if (value != 0) {
        result.lowIndex = scan1(value, 0);
        result.highIndex = scan0(value, result.lowIndex) - 1;
        result.value = maskFromSlice(result.highIndex, result.lowIndex);
    } else {
        result.lowIndex = -1;
        result.highIndex = -1;
        result.value = 0;
    }
    return result;
}

big_int cvtInt(const char *s, unsigned base) {
    big_int rv;

    while (*s) {
        switch (*s) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                rv *= base;
                rv += *s - '0';
                break;
            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
                rv *= base;
                rv += *s - 'a' + 10;
                break;
            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 'E':
            case 'F':
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

}  // namespace P4::Util

namespace P4 {

void dump(const big_int &i) {
    std::cout << i << " (";
    // DANGER -- printing a negative big_int in hex crashes...
    if (i >= 0)
        std::cout << "0x" << std::hex << i << std::dec;
    else
        std::cout << "~0x" << std::hex << ~i << std::dec;
    std::cout << ")" << std::endl;
}
void dump(const big_int *i) { dump(*i); }

}  // namespace P4
