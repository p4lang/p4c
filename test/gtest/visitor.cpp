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
#include "helpers.h"
#include "ir/ir.h"
#include "ir/visitor.h"
#include "ir/pass_manager.h"
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

TEST_F(P4C_IR, VisitorRefProcess) {
    struct TestModif : Modifier {
        bool preorder(IR::AssignmentStatement *asgn) {
            asgn->right = new IR::Cast(IR::Type_Bits::get(32, false), asgn->right);
            return true;
        }
    };
    PassManager::VisitorRef ref(new TestModif());
    auto *node = new IR::AssignmentStatement(new IR::PathExpression(IR::ID("foo")),
                                             new IR::Constant(42));
    auto *out = ref.process(node);
    ASSERT_TRUE(out);
    auto *asgn = out->to<IR::AssignmentStatement>();
    ASSERT_TRUE(asgn);

    EXPECT_EQ(asgn->left, node->left);
    EXPECT_NE(asgn->right, node->right);
    auto *cast = asgn->right->to<IR::Cast>();
    ASSERT_TRUE(cast);
    EXPECT_TRUE(cast->destType->is<IR::Type_Bits>());
    EXPECT_EQ(cast->expr, node->right);
}

}  // namespace Test
