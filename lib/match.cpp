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

#include "match.h"

static int chkmask(const match_t &m, int maskbits) {
    uintmax_t mask = (1U << maskbits) - 1;
    int shift = 0;
    while (mask && ((m.word0|m.word1) >> shift)) {
        if ((mask & m.word0 & m.word1) && (mask & m.word0 & m.word1) != mask)
            return -1;
        mask <<= maskbits;
        shift += maskbits; }
    return shift - maskbits;
}

std::ostream &operator<<(std::ostream &out, match_t m) {
    if (!m) {
        out << "*";
        return out;
    }
    int shift, bits;
    if ((shift = chkmask(m, (bits = 4))) >= 0)
        out << "0x";
    else if ((shift = chkmask(m, (bits = 3))) >= 0)
        out << "0o";
    else if ((shift = chkmask(m, (bits = 1))) >= 0)
        out << "0b";
    else
        shift = 0, bits = 1;
    uintmax_t mask = uintmax_t((1U << bits) - 1) << shift;
    for (; mask; shift -= bits, mask >>= bits)
        if (mask & m.word0 & m.word1)
            out << '*';
        else
            out << "0123456789abcdef"[(m.word1 & mask) >> shift];
    return out;
}

bool operator>>(const char *p, match_t &m) {
    if (!p) return false;
    match_t rv;
    unsigned base = 10;
    if (*p == '0') {
        switch (p[1]) {
        case 'x': case 'X': base = 16; p += 2; break;
        case 'o': case 'O': base = 8; p += 2; break;
        case 'b': case 'B': base = 2; p += 2; break;
        default: break; } }
    while (*p) {
        int digit = base - 1, mask = base - 1;
        switch (*p++) {
        case '_': continue;
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            digit = p[-1] - '0';
            break;
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
            digit = p[-1] - 'a' + 10;
            break;
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
            digit = p[-1] - 'A' + 10;
            break;
        case '*':
            if (base == 10) return false;
            mask = 0;
            break;
        default:
            return false; }
        rv.word0 *= base;
        rv.word1 *= base;
        rv.word0 |= digit ^ mask;
        rv.word1 |= digit; }
    if (base == 10) rv.word0 = ~rv.word1;
    m = rv;
    return true;
}
