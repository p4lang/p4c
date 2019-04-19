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
#include "lib/source_file.h"

namespace Test {

class P4C_IR : public P4CTest { };

TEST_F(P4C_IR, Transform) {
    struct TestTrans : public Transform {
        explicit TestTrans(IR::Constant* c) : c(c) { }

        IR::Node* postorder(IR::Add* a) override {
            EXPECT_EQ(c, a->left);
            EXPECT_EQ(c, a->right);
            return a;
        }

        IR::Constant* c;
    };

    auto c = new IR::Constant(2);
    IR::Expression* e = new IR::Add(Util::SourceInfo(), c, c);
    auto* n = e->apply(TestTrans(c));
    EXPECT_EQ(e, n);
}

}  // namespace Test
