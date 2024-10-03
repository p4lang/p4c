/*
Copyright 2024-present Intel Corporation.

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

#include "ir/ir-traversal.h"

#include <gtest/gtest.h>

#include "ir/ir.h"

namespace P4::Test {

struct TraversalTest : public ::testing::Test {};

TEST_F(TraversalTest, SimpleIRApply) {
    const auto *v = new IR::Constant(42);
    const auto *m = IR::Traversal::apply(v, &IR::Constant::value, IR::Traversal::Assign(4));
    EXPECT_NE(v, m);
    EXPECT_EQ(v->value, 42);
    EXPECT_EQ(m->value, 4);

    const auto *t = IR::Type_Bits::get(4);
    const auto *m2 = IR::Traversal::apply(v, &IR::Constant::type, IR::Traversal::Assign(t));
    EXPECT_NE(v, m2);
    EXPECT_EQ(v->value, 42);
    EXPECT_EQ(m2->value, 42);
    EXPECT_EQ(m2->type, t);
}

TEST_F(TraversalTest, SimpleIRModify) {
    auto *v = new IR::Constant(42);
    auto *m = IR::Traversal::modify(v, &IR::Constant::value, IR::Traversal::Assign(4));
    EXPECT_EQ(v, m);
    EXPECT_EQ(v->value, 4);

    const auto *t = IR::Type_Bits::get(4);
    const auto *m2 = IR::Traversal::modify(v, &IR::Constant::type, IR::Traversal::Assign(t));
    EXPECT_EQ(v, m2);
    EXPECT_EQ(v->value, 4);
    EXPECT_EQ(v->type, t);
}

TEST_F(TraversalTest, ComplexIRModify) {
    const auto *path = new IR::Path("foo");
    const auto *pe = new IR::PathExpression(path);
    const auto *add = new IR::Add(pe, new IR::Constant(42));
    auto *asgn = new IR::AssignmentStatement(pe, add);
    IR::StatOrDecl *stmt = asgn;

    modify(stmt, RTTI::to<IR::AssignmentStatement>, &IR::AssignmentStatement::right,
           RTTI::to<IR::Operation_Binary>, &IR::Operation_Binary::left,
           RTTI::to<IR::PathExpression>, &IR::PathExpression::path, &IR::Path::name, &IR::ID::name,
           IR::Traversal::Assign(P4::cstring("bar")));
    // original not modified
    EXPECT_EQ(path->name.name, "foo");
    EXPECT_EQ(asgn->left->checkedTo<IR::PathExpression>()->path->name.name, "foo");
    EXPECT_EQ(
        asgn->right->checkedTo<IR::Add>()->left->checkedTo<IR::PathExpression>()->path->name.name,
        "bar");
}

struct S0 {
    int v0 = 0;
    int v1 = 1;
};

struct S1 {
    int v0 = 2;
    S0 s0;
};

TEST_F(TraversalTest, SimpleNonIRModify) {
    S1 s1;
    modify(&s1, &S1::s0, &S0::v1, IR::Traversal::Assign(4));
    EXPECT_EQ(s1.s0.v1, 4);
    IR::Traversal::modify(&s1, &S1::s0, &S0::v0, [](auto *v) {
        ++*v;
        return v;
    });
    EXPECT_EQ(s1.s0.v0, 1);
}

}  // namespace P4::Test
