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
#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/exceptions.h"

TEST(IR, Equiv) {
    auto *t = IR::Type::Bits::get(16);
    auto *a1 = new IR::Constant(t, 10);
    auto *a2 = new IR::Constant(t, 10);
    auto *b = new IR::Constant(IR::Type::Bits::get(10), 10);
    auto *c = new IR::Constant(t, 20);
    auto *d1 = new IR::PathExpression("d");
    auto *d2 = new IR::PathExpression("d");
    auto *e = new IR::PathExpression("e");
    auto *d1m = new IR::Member(d1, "m");
    auto *d2m = new IR::Member(d2, "m");
    auto *em = new IR::Member(e, "m");
    auto *d1f = new IR::Member(d1, "f");

    EXPECT_TRUE(a1->equiv(*a2));
    EXPECT_TRUE(d1->equiv(*d2));
    EXPECT_FALSE(a1->equiv(*b));
    EXPECT_FALSE(a1->equiv(*c));
    EXPECT_FALSE(a1->equiv(*e));
    EXPECT_FALSE(d1->equiv(*e));
    EXPECT_TRUE(d1m->equiv(*d2m));
    EXPECT_FALSE(d1m->equiv(*em));
    EXPECT_FALSE(d1m->equiv(*d1f));

    auto *call1 = new IR::MethodCallExpression(d1m, { a1, d1 });
    auto *call2 = new IR::MethodCallExpression(d2m, { a2, d2 });
    auto *call3 = new IR::MethodCallExpression(d1m, { b, d1 });

    EXPECT_TRUE(call1->equiv(*call2));
    EXPECT_FALSE(call1->equiv(*call3));

    auto *list1 = new IR::ListExpression({ a1, b, d1 });
    auto *list2 = new IR::ListExpression({ a1, b, d2 });
    auto *list3 = new IR::ListExpression({ a1, b, e });

    EXPECT_TRUE(list1->equiv(*list2));
    EXPECT_FALSE(list1->equiv(*list3));

    auto *pr1 = new IR::V1Program;
    auto *pr2 = pr1->clone();
    pr1->add("a", a1);
    pr1->add("b", b);
    pr1->add("call", call1);
    pr2->add("a", a2);
    pr2->add("b", b);
    pr2->add("call", call2);
    EXPECT_TRUE(pr1->equiv(*pr2));
    pr1->add("lista", list1);
    pr2->add("listb", list1);
    EXPECT_FALSE(pr1->equiv(*pr2));
}
