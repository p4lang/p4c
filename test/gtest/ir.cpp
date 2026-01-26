/*
Copyright 2025-present Altera Corporation.

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

#include "ir/ir.h"

#include <gtest/gtest.h>

#include "helpers.h"

namespace P4::Test {

struct IrTest : P4CTest {};
struct ConstantTest : IrTest {};

TEST_F(ConstantTest, ConstantInt0) {
    RedirectStderr errors;
    auto con = IR::Constant::get(IR::Type_Bits::get(0, true), 1);
    errors.dumpAndReset();
    EXPECT_TRUE(errors.contains("warn=overflow"));
    EXPECT_TRUE(errors.contains("cannot have non-zero value"));
    EXPECT_EQ(P4::warningCount(), 1);
    EXPECT_EQ(con->value, 0);
}

TEST_F(ConstantTest, ConstantInt1Overflow) {
    RedirectStderr errors;
    auto con = IR::Constant::get(IR::Type_Bits::get(1, true), 1);
    errors.dumpAndReset();
    EXPECT_TRUE(errors.contains("warn=overflow"));
    EXPECT_TRUE(errors.contains("signed value does not fit"));
    EXPECT_EQ(P4::warningCount(), 1);
    EXPECT_EQ(con->value, -1);
}

TEST_F(ConstantTest, ConstantInt1NoOverflow) {
    auto con = IR::Constant::get(IR::Type_Bits::get(1, true), 0);
    EXPECT_EQ(P4::warningCount(), 0);
    EXPECT_EQ(con->value, 0);
}

TEST_F(ConstantTest, ConstantBit1Overflow) {
    RedirectStderr errors;
    auto con = IR::Constant::get(IR::Type_Bits::get(1, false), 2);
    errors.dumpAndReset();
    EXPECT_TRUE(errors.contains("warn=overflow"));
    EXPECT_TRUE(errors.contains("value does not fit"));
    EXPECT_EQ(P4::warningCount(), 1);
    EXPECT_EQ(con->value, 0);
}

TEST_F(ConstantTest, ConstantBit1OverflowNeg) {
    RedirectStderr errors;
    auto con = IR::Constant::get(IR::Type_Bits::get(1, false), -1);
    errors.dumpAndReset();
    EXPECT_TRUE(errors.contains("warn=mismatch"));
    EXPECT_TRUE(errors.contains("negative value with unsigned type"));
    EXPECT_EQ(P4::warningCount(), 1);
    EXPECT_EQ(con->value, 1);
}

TEST_F(ConstantTest, ConstantBit1NoOverflow) {
    auto con = IR::Constant::get(IR::Type_Bits::get(1, false), 1);
    EXPECT_EQ(P4::warningCount(), 0);
    EXPECT_EQ(con->value, 1);
}

}  // namespace P4::Test
