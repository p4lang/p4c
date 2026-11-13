// SPDX-FileCopyrightText: 2025 Altera Corporation
// Copyright 2025-present Altera Corporation.
//
// SPDX-License-Identifier: Apache-2.0

#include "ir/ir.h"

#include <gtest/gtest.h>

#include <unordered_set>

#include "helpers.h"
#include "ir/compare.h"

namespace P4::Test {

struct IrTest : P4CTest {};
struct ConstantTest : IrTest {};

TEST_F(IrTest, SymbolicVariableHashIgnoresType) {
    const IR::SymbolicVariable first(IR::Type_Bits::get(8), "value"_cs);
    const IR::SymbolicVariable alias(IR::Type_Boolean::get(), "value"_cs);
    const IR::SymbolicVariable other(IR::Type_Bits::get(8), "other"_cs);
    const IR::SymbolicVariableHash hash;
    EXPECT_TRUE(IR::SymbolicVariableEqual{}(first, alias));
    EXPECT_EQ(hash(first), hash(alias));
    EXPECT_EQ(hash(first), hash(&first));

    std::unordered_set<IR::SymbolicVariable, IR::SymbolicVariableHash, IR::SymbolicVariableEqual>
        values{first, alias, other};
    EXPECT_EQ(values.size(), 2U);
    EXPECT_EQ(values.count(alias), 1U);
    std::unordered_set<const IR::SymbolicVariable *, IR::SymbolicVariableHash,
                       IR::SymbolicVariableEqual>
        pointers{&first, &other};
    EXPECT_EQ(pointers.count(&alias), 1U);
}

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
