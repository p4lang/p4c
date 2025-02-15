/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "backends/tofino/bf-asm/gtest/register-matcher.h"

#include <cctype>
#include <iostream>

namespace BfAsm {

namespace Test {

RegisterMatcher::RegisterMatcher(const char *spec) : bitsize(0) {
    enum State {
        INIT,
        IDENT,
        WIDTH,
        BIN_VALUE,
        OCT_VALUE,
        HEX_VALUE,
    } state(INIT);
    uint32_t width(0);
    bitvec value;
    uint8_t digit(0);
    bool negate(false);

    while (true) {
        switch (state) {
            case INIT:
                if (std::isdigit(*spec)) {
                    width = *spec - '0';
                    state = WIDTH;
                } else if (*spec == '~') {
                    negate = !negate;
                } else if (std::isalpha(*spec) || *spec == '_') {
                    /* -- ignore identifiers in the spec */
                    state = IDENT;
                }
                break;

            case IDENT:
                if (*spec == '~') {
                    state = INIT;
                    negate = true;
                } else if (!std::isalpha(*spec) && !std::isdigit(*spec) && *spec != '_') {
                    state = INIT;
                    negate = false;
                }
                break;

            case WIDTH:
                if (std::isdigit(*spec)) {
                    width = 10 * width + *spec - '0';
                } else if (*spec == 'b') {
                    state = BIN_VALUE;
                    value = bitvec();
                } else if (*spec == 'x') {
                    state = HEX_VALUE;
                    value = bitvec();
                } else if (*spec == 'o') {
                    state = OCT_VALUE;
                    value = bitvec();
                }
                break;

            case BIN_VALUE:
                if (*spec == '0' || *spec == '1') {
                    digit = *spec - '0';
                    if (negate) digit = ~digit;
                    value <<= 1;
                    value |= bitvec(digit & 0x01);
                } else if (*spec == '|' || *spec == 0) {
                    pushBits(value, width);
                    state = INIT;
                    negate = false;
                }
                break;

            case HEX_VALUE:
                if (std::isxdigit(*spec)) {
                    if (*spec >= '0' && *spec <= '9') {
                        digit = *spec - '0';
                    } else if (*spec >= 'a' && *spec <= 'f') {
                        digit = *spec - 'a' + 10;
                    } else if (*spec >= 'A' && *spec <= 'F') {
                        digit = *spec - 'A' + 10;
                    }
                    if (negate) digit = ~digit;
                    value <<= 4;
                    value |= bitvec(digit & 0x0f);
                } else if (*spec == '|' || *spec == 0) {
                    pushBits(value, width);
                    state = INIT;
                    negate = false;
                }
                break;

            case OCT_VALUE:
                if (*spec >= '0' && *spec <= '7') {
                    digit = *spec - '0';
                    if (negate) digit = ~digit;
                    value <<= 3;
                    value |= bitvec(digit & 0x07);
                } else if (*spec == '|' || *spec == 0) {
                    pushBits(value, width);
                    state = INIT;
                    negate = false;
                }
                break;
        }

        if (*spec == 0) break;
        ++spec;
    }
}

void RegisterMatcher::pushBits(const bitvec &bits, uint32_t width) {
    expected <<= width;
    bitvec mask;
    mask.setrange(0, width);
    expected |= bits & mask;
    bitsize += width;
}

bool RegisterMatcher::checkRegister(std::ostream &os, const uint8_t reg[], uint32_t rsize) const {
    const uint32_t bytesize((bitsize + 7) / 8);
    if (rsize < bytesize) {
        os << "checked register is shorter than the expected value";
        return false;
    }

    uint32_t bitindex(0);
    bool fail(false);
    for (int i(0); i < rsize; ++i) {
        const uint8_t byte(expected.getrange(bitindex, 8));
        fail = (byte != reg[i]) || fail;
        bitindex += 8;
    }

    if (fail) {
        os << std::hex << std::setfill('0');
        os << "  expected: ";
        for (auto i(rsize); i > 0; --i) {
            uint8_t byte(expected.getrange((i - 1) * 8, 8));
            os << ' ' << std::setw(2) << static_cast<unsigned int>(byte);
            bitindex += 8;
        }
        os << '\n';
        os << "    actual: ";
        for (auto i(rsize); i > 0; --i) {
            os << ' ' << std::setw(2) << static_cast<unsigned int>(reg[i - 1]);
        }
        os << '\n';
    }

    return !fail;
}

}  // namespace Test

}  // namespace BfAsm
