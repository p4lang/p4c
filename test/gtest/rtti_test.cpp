/*
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

#include <sstream>

#include "gtest/gtest.h"
#include "ir/ir.h"
#include "ir/json_loader.h"
#include "ir/node.h"
#include "ir/vector.h"

namespace Test {

TEST(RTTI, TypeId) {
    auto *c = new IR::Constant(2);
    IR::Expression *e1 = new IR::Add(Util::SourceInfo(), c, c);

    EXPECT_EQ(c->typeId(), IR::NK_Constant);
    EXPECT_EQ(e1->typeId(), IR::NK_Add);
    EXPECT_NE(e1->typeId(), IR::NK_Expression);

    auto *v = new IR::Vector<IR::Type>();
    EXPECT_EQ(v->typeId(), IR::NK_VectorT | IR::Type::static_typeId());
}

TEST(RTTI, Is) {
    IR::Node *c = new IR::Constant(2);

    EXPECT_TRUE(c->is<IR::CompileTimeValue>());
    EXPECT_TRUE(c->is<IR::Expression>());
    EXPECT_TRUE(c->is<IR::Literal>());
    EXPECT_FALSE(c->is<IR::Declaration>());

    IR::Node *v = new IR::Vector<IR::Type>();
    EXPECT_TRUE(v->is<IR::VectorBase>());

    IR::Node *iv = new IR::IndexedVector<IR::Parameter>();
    EXPECT_TRUE(iv->is<IR::Vector<IR::Parameter>>());
}

TEST(RTTI, Casts) {
    auto *c = new IR::Constant(2);
    IR::Expression *e1 = new IR::Add(Util::SourceInfo(), c, c);

    EXPECT_NE(c->to<IR::Literal>(), nullptr);
    EXPECT_NE(e1->to<IR::Operation_Binary>(), nullptr);
    EXPECT_EQ(e1->to<IR::Declaration>(), nullptr);

    IR::Node *iv = new IR::IndexedVector<IR::Parameter>();
    EXPECT_NE(iv->to<IR::Vector<IR::Parameter>>(), nullptr);
    EXPECT_EQ(c->to<IR::Vector<IR::Parameter>>(), nullptr);
}

TEST(RTTI, JsonRestore) {
    auto *c = new IR::Constant(2);
    IR::Expression *e1 = new IR::Add(Util::SourceInfo(), c, c);

    std::stringstream ss;
    JSONGenerator(ss) << e1 << '\n';

    JSONLoader loader(ss);

    const IR::Node *e2 = nullptr;
    loader >> e2;

    EXPECT_EQ(e2->typeId(), IR::NK_Add);
}

}  // namespace Test
