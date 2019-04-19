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

#include "gtest/gtest.h"
#include "helpers.h"
#include "ir/ir.h"

namespace Test {

class ConstantExpr : public P4CTest { };

TEST_F(ConstantExpr, TestInt) {
    int val = 0x1;
    IR::Constant c(val);
    auto res = c.asInt();
    EXPECT_EQ(res, val);

    val = INT32_MAX;
    IR::Constant m(val);
    res = m.asInt();
    EXPECT_EQ(res, val);

    val = 0;
    IR::Constant z(val);
    res = z.asInt();
    EXPECT_EQ(res, val);

    val = -1;
    IR::Constant mo(val);
    res = mo.asInt();
    EXPECT_EQ(res, val);

    val = -10000000;
    IR::Constant mr(val);
    res = mr.asInt();
    EXPECT_EQ(res, val);
}

TEST_F(ConstantExpr, TestUInt) {
    unsigned int val = 0x12345678U;
    IR::Constant c(val);
    auto res = c.asUnsigned();
    EXPECT_EQ(res, val);

    val = UINT32_MAX;
    IR::Constant m(val);
    res = m.asUnsigned();
    EXPECT_EQ(res, val);

    val = 0;
    IR::Constant z(val);
    res = z.asUnsigned();
    EXPECT_EQ(res, val);
}

TEST_F(ConstantExpr, TestLong) {
    long val = 0x12345678L;
    IR::Constant c(val);
    auto res = c.asLong();
    EXPECT_EQ(res, val);

    /* long is inconsistent! */
#if __WORDSIZE == 64
    val = INT64_MAX;
#else
    val = INT32_MAX;
#endif
    IR::Constant m(val);
    res = m.asLong();
    EXPECT_EQ(res, val);

    val = 0L;
    IR::Constant z(val);
    res = z.asLong();
    EXPECT_EQ(res, val);

    val = -1L;
    IR::Constant mo(val);
    res = mo.asLong();
    EXPECT_EQ(res, val);

    val = -100L;
    IR::Constant mr(val);
    res = mr.asLong();
    EXPECT_EQ(res, val);
}

TEST_F(ConstantExpr, TestUint64) {
    uint64_t val = UINT64_C(0x100000000);
    IR::Constant c(val);
    auto res = c.asUint64();
    EXPECT_EQ(res, val);

    val = UINT64_MAX;
    IR::Constant m(val);
    res = m.asUint64();
    EXPECT_EQ(res, val);

    val = UINT64_C(0);
    IR::Constant z(val);
    res = z.asUint64();
    EXPECT_EQ(res, val);
}

TEST_F(ConstantExpr, TestIntegerFuncs) {
    unsigned int val1 = 0x03030303U;
    unsigned int val2 = 0x06060606U;

    IR::Constant c1(val1);
    IR::Constant c2(val2);

    auto lsh_res = c1 << 1;
    auto rsh_res = c2 >> 1;
    auto and_res = c1 & c2;
    auto ior_res = c1 | c2;
    auto xor_res = c1 ^ c2;
    auto sub_res = c2 - c1;

    EXPECT_EQ(lsh_res.asUnsigned(), 0x06060606U);
    EXPECT_EQ(rsh_res.asUnsigned(), 0x03030303U);
    EXPECT_EQ(and_res.asUnsigned(), 0x02020202U);
    EXPECT_EQ(ior_res.asUnsigned(), 0x07070707U);
    EXPECT_EQ(xor_res.asUnsigned(), 0x05050505U);
    EXPECT_EQ(sub_res.asUnsigned(), 0x03030303U);

    IR::Constant c(int(123));
    auto neg_res = -c;
    EXPECT_EQ(neg_res.asInt(), -123);
}

}  // namespace Test
