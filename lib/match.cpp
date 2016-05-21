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

#include <assert.h>
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
