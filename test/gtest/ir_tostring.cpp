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

#include <iostream>
#include <vector>

#include "gtest/gtest.h"
#include "ir/ir.h"

TEST(IR, BinOpToString) {
    auto c = new IR::Constant(2);
    auto f = new IR::Member(new IR::PathExpression("obj"), "f");

    EXPECT_STREQ("2", c->toString());
    EXPECT_STREQ("obj.f", f->toString());

    // Binary operations printed "x OP y".
    std::vector<IR::Operation_Binary*> ops = {
        new IR::Mul(f, c),
        new IR::Div(f, c),
        new IR::Mod(f, c),
        new IR::Add(f, c),
        new IR::Sub(f, c),
        new IR::AddSat(f, c),
        new IR::SubSat(f, c),
        new IR::Shl(f, c),
        new IR::Shr(f, c),
        new IR::Equ(f, c),
        new IR::Neq(f, c),
        new IR::Lss(f, c),
        new IR::Leq(f, c),
        new IR::Grt(f, c),
        new IR::Geq(f, c),
        new IR::BAnd(f, c),
        new IR::BOr(f, c),
        new IR::BXor(f, c),
        new IR::LAnd(f, c),
        new IR::LOr(f, c),
        new IR::Concat(f, c),
        new IR::Mask(f, c),
    };
    for (auto* op : ops) {
        std::string expected = "obj.f " + op->getStringOp() + " 2";
        EXPECT_STREQ(expected.c_str(), op->toString());
    }

    // Array indexing is printed "left[right]".
    EXPECT_STREQ("obj.f[2]", IR::ArrayIndex(f, c).toString());

    // Range is printed "left..right".
    EXPECT_STREQ("obj.f..2", IR::Range(f, c).toString());
}
