// SPDX-FileCopyrightText: 2024 Intel Corporation.
// Copyright 2024-present Intel Corporation.
//
// SPDX-License-Identifier: Apache-2.0

#include "ir/ir-traversal.h"

#include <gtest/gtest.h>

#include "ir/ir.h"

namespace P4::Test {

struct TraversalTest : public ::testing::Test {};

TEST_F(TraversalTest, SimpleIRApply) {
    IR::Ptr<IR::Constant> v = new IR::Constant(42);
    auto m = IR::Traversal::apply(v, &IR::Constant::value, IR::Traversal::Assign(4));
    EXPECT_NE(v, m);
    EXPECT_EQ(v->value, 42);
    EXPECT_EQ(m->value, 4);

    auto t = IR::Type_Bits::get(4);
    auto m2 = IR::Traversal::apply(v, &IR::Constant::type, IR::Traversal::Assign(t));
    EXPECT_NE(v, m2);
    EXPECT_EQ(v->value, 42);
    EXPECT_EQ(m2->value, 42);
    EXPECT_EQ(m2->type, t);
}

TEST_F(TraversalTest, SimpleIRModify) {
    IR::Constant constant(42);
    auto *v = &constant;
    auto *m = IR::Traversal::modify(v, &IR::Constant::value, IR::Traversal::Assign(4));
    EXPECT_EQ(v, m);
    EXPECT_EQ(v->value, 4);

    auto t = IR::Type_Bits::get(4);
    const auto *m2 = IR::Traversal::modify(v, &IR::Constant::type, IR::Traversal::Assign(t));
    EXPECT_EQ(v, m2);
    EXPECT_EQ(v->value, 4);
    EXPECT_EQ(v->type, t);
}

TEST_F(TraversalTest, ComplexIRModify) {
    IR::Ptr<IR::Path> path = new IR::Path("foo");
    IR::Ptr<IR::PathExpression> pe = new IR::PathExpression(path);
    IR::Ptr<IR::Add> add = new IR::Add(pe, new IR::Constant(42));
    IR::AssignmentStatement assignment(pe, add);
    auto *asgn = &assignment;
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

#if !HAVE_LIBGC
namespace {
class TrackedTraversalConstant final : public IR::Constant {
    int &live;

 public:
    TrackedTraversalConstant(int &live, int value) : IR::Constant(value), live(live) { ++live; }
    TrackedTraversalConstant(const TrackedTraversalConstant &other)
        : IR::Constant(other), live(other.live) {
        ++live;
    }
    ~TrackedTraversalConstant() override { --live; }
    TrackedTraversalConstant *clone() const override { return new TrackedTraversalConstant(*this); }
};
}  // namespace

TEST_F(TraversalTest, ReplacementReleasesDiscardedClones) {
    int live = 0;
    {
        IR::Ptr<IR::Constant> original = new TrackedTraversalConstant(live, 1);
        IR::Ptr<IR::Constant> replacement = new TrackedTraversalConstant(live, 2);
        auto result = IR::Traversal::apply(original, IR::Traversal::Assign(replacement));
        EXPECT_EQ(result, replacement);
        EXPECT_EQ(live, 2);
        IR::Ptr<IR::Add> sum = new IR::Add(original, original);
        auto changed =
            IR::Traversal::apply(sum, &IR::Add::left, IR::Traversal::Assign(replacement));
        EXPECT_EQ(changed->left, replacement);
        EXPECT_EQ(changed->right, original);
        EXPECT_EQ(sum->left, original);
        EXPECT_EQ(live, 2);
    }
    EXPECT_EQ(live, 0);
}
#endif

}  // namespace P4::Test
