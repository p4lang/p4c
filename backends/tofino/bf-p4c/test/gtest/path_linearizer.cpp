/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include "gtest/gtest.h"

#include "bf-p4c/midend/path_linearizer.h"
#include "ir/ir.h"

namespace BFN {

TEST(TofinoPathLinearizer, PathExpression) {
    // A lone PathExpression should be linearized as a single component.
    auto* pathExpr = new IR::PathExpression(new IR::Path(IR::ID("path")));

    PathLinearizer linearizer;
    pathExpr->apply(linearizer);

    ASSERT_TRUE(bool(linearizer.linearPath));
    ASSERT_EQ(1u, linearizer.linearPath->components.size());
    EXPECT_EQ(pathExpr, linearizer.linearPath->components[0]);
}

TEST(TofinoPathLinearizer, MemberExpression) {
    // A single Member should be linearized as two components: the Member, and
    // the PathExpression it contains.
    auto* pathExpr = new IR::PathExpression(new IR::Path(IR::ID("path")));
    auto* member = new IR::Member(pathExpr, IR::ID("member"));

    PathLinearizer linearizer;
    member->apply(linearizer);

    ASSERT_TRUE(bool(linearizer.linearPath));
    ASSERT_EQ(2u, linearizer.linearPath->components.size());
    EXPECT_EQ(pathExpr, linearizer.linearPath->components[0]);
    EXPECT_EQ(member, linearizer.linearPath->components[1]);
}

TEST(TofinoPathLinearizer, NestedMemberExpressions) {
    // Nested Member expressions should be linearized into a sequence of
    // components that matches the components that the corresponding P4
    // expression would have.
    auto* first = new IR::PathExpression(new IR::Path(IR::ID("first")));
    auto* second = new IR::Member(first, IR::ID("second"));
    auto* third = new IR::Member(second, IR::ID("third"));
    auto* fourth = new IR::Member(third, IR::ID("fourth"));

    PathLinearizer linearizer;
    fourth->apply(linearizer);

    ASSERT_TRUE(bool(linearizer.linearPath));
    ASSERT_EQ(4u, linearizer.linearPath->components.size());
    EXPECT_EQ(first, linearizer.linearPath->components[0]);
    EXPECT_EQ(second, linearizer.linearPath->components[1]);
    EXPECT_EQ(third, linearizer.linearPath->components[2]);
    EXPECT_EQ(fourth, linearizer.linearPath->components[3]);
}

TEST(TofinoPathLinearizer, RejectUnexpectedExpressions) {
    // If there are any expression types we don't expect in the path, we should
    // fail to linearize it.
    auto* first = new IR::PathExpression(new IR::Path(IR::ID("first")));
    auto* second = new IR::Member(first, IR::ID("second"));
    auto* cast = new IR::Cast(IR::Type::Bits::get(8), second);
    auto* third = new IR::Member(cast, IR::ID("third"));

    PathLinearizer linearizer;
    third->apply(linearizer);

    ASSERT_FALSE(bool(linearizer.linearPath));
}

TEST(TofinoPathLinearizer, HeaderStackIndexing) {
    // For now, at least, PathLinearizer doesn't support indexing into header
    // stacks.
    // Note: the linearized path is not correct.
    auto* first = new IR::PathExpression(new IR::Path(IR::ID("first")));
    auto* second = new IR::Member(first, IR::ID("second"));
    auto* index = new IR::ArrayIndex(second, new IR::Constant(0, 10));
    auto* third = new IR::Member(index, IR::ID("third"));

    PathLinearizer linearizer;
    third->apply(linearizer);

    ASSERT_TRUE(bool(linearizer.linearPath));
}

TEST(TofinoPathLinearizer, RejectNonPathlikeExpressions) {
    // Attempting to linearize an expression which isn't path-like should throw
    // an exception.
    PathLinearizer linearizer;

    // A path by itself isn't path-like.
    auto* path = new IR::Path(IR::ID("first"));
    ASSERT_ANY_THROW(path->apply(linearizer));

    // Most expressions aren't path-like.
    auto* addExpr = new IR::Add(new IR::Constant(0, 10), new IR::Constant(0, 10));
    ASSERT_ANY_THROW(addExpr->apply(linearizer));

    // A statement isn't path-like.
    auto* pathExpr = new IR::PathExpression(new IR::Path(IR::ID("path")));
    auto* statement =
      new IR::AssignmentStatement(pathExpr, new IR::Constant(0, 10));
    ASSERT_ANY_THROW(statement->apply(linearizer));

    // A control isn't path-like.
    auto* controlType =
      new IR::Type_Control(IR::ID("control"), new IR::ParameterList);
    auto* control =
      new IR::P4Control(IR::ID("control"), controlType, new IR::BlockStatement);
    ASSERT_ANY_THROW(control->apply(linearizer));
}

}  // namespace BFN
