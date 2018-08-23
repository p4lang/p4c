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
#include <stdint.h>
#include "hex.h"

std::ostream &operator<<(std::ostream &os, const hexvec &h) {
    auto save = os.flags();
    auto save_fill = os.fill();
    char *p = reinterpret_cast<char *>(h.data);
    for (size_t i = 0; i < h.len; i++, p += h.elsize) {
        os << (i ? ' ' : '[');
        uintmax_t val;
        switch (h.elsize) {
        case 1:
            val = *reinterpret_cast<uint8_t *>(p);
            break;
        case 2:
            val = *reinterpret_cast<uint16_t *>(p);
            break;
        case 4:
            val = *reinterpret_cast<uint32_t *>(p);
            break;
        case 8:
            val = *reinterpret_cast<uint64_t *>(p);
            break;
        default:
            val = *reinterpret_cast<uintmax_t *>(p);
            val &= ~(~0ULL << 8*h.elsize); }
        os << std::hex << std::setw(h.width) << std::setfill(h.fill) << val; }
    os << ']';
    os.fill(save_fill);
    os.flags(save);
    return os;
}
