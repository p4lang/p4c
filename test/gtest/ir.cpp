// SPDX-FileCopyrightText: 2025 Altera Corporation
// Copyright 2025-present Altera Corporation.
//
// SPDX-License-Identifier: Apache-2.0

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
