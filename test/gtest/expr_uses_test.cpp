// SPDX-FileCopyrightText: 2017 Barefoot Networks, Inc.
// Copyright 2017-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "midend/expr_uses.h"

#include <gtest/gtest.h>

#include "ir/ir.h"

using namespace P4;
using namespace P4::literals;

TEST(expr_uses, expr_uses) {
    auto obj1 = new IR::PathExpression("obj1");
    auto obj2 = new IR::PathExpression("obj2");
    auto obj1_f1 = new IR::Member(obj1, "f1");
    auto obj1_f11 = new IR::Member(obj1, "f11");
    auto obj2_f1 = new IR::Member(obj2, "f1");
    auto obj2_f11 = new IR::Member(obj2, "f11");
    auto add = new IR::Add(obj1_f1, obj2_f11);
    auto sub = new IR::Sub(obj1_f11, obj2_f1);
    auto add2 = new IR::Add(obj1_f1, obj1_f11);

    EXPECT_TRUE(exprUses(add, "obj1"_cs));
    EXPECT_TRUE(exprUses(add, "obj2"_cs));
    EXPECT_TRUE(exprUses(add, "obj1.f1"_cs));
    EXPECT_TRUE(exprUses(add, "obj1.f1[0]"_cs));
    EXPECT_FALSE(exprUses(add, "obj1.f11"_cs));
    EXPECT_FALSE(exprUses(add, "obj1.f11[1]"_cs));
    EXPECT_FALSE(exprUses(add, "obj2.f1"_cs));
    EXPECT_TRUE(exprUses(add, "obj2.f11"_cs));
    EXPECT_TRUE(exprUses(sub, "obj1"_cs));
    EXPECT_TRUE(exprUses(sub, "obj2"_cs));
    EXPECT_TRUE(exprUses(add2, "obj1.f1"_cs));
    EXPECT_TRUE(exprUses(add2, "obj1.f11"_cs));
}
