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

#ifndef BACKENDS_TOFINO_BF_ASM_GTEST_REGISTER_MATCHER_H_
#define BACKENDS_TOFINO_BF_ASM_GTEST_REGISTER_MATCHER_H_

#include <cstdint>
#include <iosfwd>
#include <sstream>

#include "bf-asm/ubits.h"
#include "gtest/gtest.h"
#include "p4c/lib/bitvec.h"

namespace BfAsm {

namespace Test {

class RegisterMatcher {
 private:
    bitvec expected;
    uint32_t bitsize;

 public:
    explicit RegisterMatcher(const char *spec);

    bool checkRegister(std::ostream &os, const uint8_t reg[], uint32_t size) const;

    template <int N>
    bool checkRegister(std::ostream &os, const ubits<N> &bits) const {
        static_assert(N > 0 && N <= 64);
        const uint64_t value(bits);
        return checkRegister(os, reinterpret_cast<const uint8_t *>(&value), (N + 7) / 8);
    }

 private:
    void pushBits(const bitvec &bits, uint32_t width);
};

}  // namespace Test

}  // namespace BfAsm

#define EXPECT_REGISTER(reg, expected)                                                          \
    do {                                                                                        \
        RegisterMatcher matcher(expected);                                                      \
        std::ostringstream oss;                                                                 \
        if (!matcher.checkRegister(oss, reg)) {                                                 \
            ADD_FAILURE() << "check of the register " << #reg << " has failed:\n" << oss.str(); \
        }                                                                                       \
    } while (false)

#endif /* BACKENDS_TOFINO_BF_ASM_GTEST_REGISTER_MATCHER_H_ */
