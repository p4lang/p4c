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

#include <gtest/gtest.h>

#include "helpers.h"
#include "ir/ir.h"
#include "ir/irutils.h"
#include "ir/visitor.h"
#include "lib/source_file.h"

namespace Test {

class P4C_IR : public P4CTest {};

TEST_F(P4C_IR, Transform) {
    struct TestTrans : public Transform {
        explicit TestTrans(IR::Constant *c) : c(c) {}

        IR::Node *postorder(IR::Add *a) override {
            EXPECT_EQ(c, a->left);
            EXPECT_EQ(c, a->right);
            return a;
        }

        IR::Constant *c;
    };

    auto c = new IR::Constant(2);
    IR::Expression *e = new IR::Add(Util::SourceInfo(), c, c);
    auto *n = e->apply(TestTrans(c));
    EXPECT_EQ(e, n);
}

TEST_F(P4C_IR, InlineBlock) {
    struct TestTrans : public Transform {
        explicit TestTrans(const IR::Constant *c) : c(c) {}

        const IR::Node *postorder(IR::AssignmentStatement *a) override {
            a->right = c;
            return IR::inlineBlock(*this, {a, a});
        }

        const IR::Constant *c;
    };

    const auto *c2 = new IR::Constant(2);
    const auto *c4 = new IR::Constant(4);
    const auto *pe = new IR::PathExpression(new IR::Path("foo"));
    const auto *stmt = new IR::BlockStatement(IR::IndexedVector<IR::StatOrDecl>({
        new IR::AssignmentStatement(pe, c2),
        new IR::IfStatement(IR::getBoolLiteral(true), new IR::AssignmentStatement(pe, c2),
                            new IR::BlockStatement(IR::IndexedVector<IR::StatOrDecl>({
                                new IR::AssignmentStatement(pe, c2),
                            }))),
    }));

    const auto *n = stmt->apply(TestTrans(c4));
    ASSERT_TRUE(n);
    const auto *bl0 = n->to<IR::BlockStatement>();
    ASSERT_TRUE(bl0);
    ASSERT_EQ(bl0->components.size(), 3);
    EXPECT_TRUE(bl0->components[0]->is<IR::AssignmentStatement>());
    EXPECT_TRUE(bl0->components[1]->is<IR::AssignmentStatement>());
    const auto *ifs = bl0->components[2]->to<IR::IfStatement>();
    ASSERT_TRUE(ifs);
    EXPECT_TRUE(ifs->ifTrue->is<IR::BlockStatement>());
    EXPECT_TRUE(ifs->ifFalse->is<IR::BlockStatement>());
}

}  // namespace Test
