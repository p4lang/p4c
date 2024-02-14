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

#include <gtest/gtest.h>

#include <sstream>

#include "ir/ir.h"
#include "ir/json_loader.h"
#include "ir/node.h"
#include "ir/vector.h"

namespace Test {

TEST(RTTI, TypeId) {
    auto *c = new IR::Constant(2);
    IR::Expression *e1 = new IR::Add(Util::SourceInfo(), c, c);

    EXPECT_EQ(c->typeId(), IR::NodeKind::Constant);
    EXPECT_EQ(e1->typeId(), IR::NodeKind::Add);
    EXPECT_NE(e1->typeId(), IR::NodeKind::Expression);

    auto *v = new IR::Vector<IR::Type>();
    EXPECT_EQ(RTTI::typeidDiscriminator(v->typeId()), IR::NodeDiscriminator::VectorT);
    EXPECT_EQ(RTTI::innerTypeId(v->typeId()), IR::NodeKind::Type);
    EXPECT_EQ(v->typeId(),
              RTTI::combineTypeIdWithDiscriminator(RTTI::TypeId(IR::NodeDiscriminator::VectorT),
                                                   RTTI::TypeId(IR::NodeKind::Type)));
    auto *v2 = new IR::Vector<IR::Parameter>();
    EXPECT_NE(v2->typeId(), v->typeId());
}

TEST(RTTI, Is) {
    IR::Node *c = new IR::Constant(2);

    EXPECT_TRUE(c->is<IR::CompileTimeValue>());
    EXPECT_TRUE(c->is<IR::Expression>());
    EXPECT_TRUE(c->is<IR::Literal>());
    EXPECT_FALSE(c->is<IR::Declaration>());

    IR::Node *v = new IR::Vector<IR::Type>();
    IR::Node *v2 = new IR::Vector<IR::Parameter>();
    EXPECT_TRUE(v->is<IR::VectorBase>());
    EXPECT_FALSE(v->is<IR::Type>());
    EXPECT_FALSE(v2->is<IR::Vector<IR::Type>>());
    EXPECT_FALSE(v2->isA(v->typeId()));

    IR::Node *iv = new IR::IndexedVector<IR::Parameter>();
    EXPECT_TRUE(iv->is<IR::Vector<IR::Parameter>>());
    EXPECT_FALSE(v2->is<IR::IndexedVector<IR::Parameter>>());
}

TEST(RTTI, Casts) {
    auto *c = new IR::Constant(2);
    IR::Expression *e1 = new IR::Add(c, c);
    IR::Node *decl = new IR::NamedExpression("foo", e1);

    EXPECT_NE(c->to<IR::Literal>(), nullptr);
    EXPECT_NE(e1->to<IR::Operation_Binary>(), nullptr);
    EXPECT_EQ(e1->to<IR::Declaration>(), nullptr);
    EXPECT_NE(decl->to<IR::Declaration>(), nullptr);
    EXPECT_NE(decl->to<IR::IDeclaration>(), nullptr);

    IR::Node *iv = new IR::IndexedVector<IR::Parameter>();
    EXPECT_NE(iv->to<IR::Vector<IR::Parameter>>(), nullptr);
    EXPECT_EQ(c->to<IR::Vector<IR::Parameter>>(), nullptr);

    EXPECT_EQ(c->to<IR::Literal>(), dynamic_cast<IR::Literal *>(c));
    EXPECT_EQ(decl->to<IR::Declaration>(), dynamic_cast<IR::Declaration *>(decl));
    EXPECT_EQ(decl->to<IR::IDeclaration>(), dynamic_cast<IR::IDeclaration *>(decl));
    auto *id = decl->to<IR::IDeclaration>();
    EXPECT_EQ(id->to<IR::Declaration>(), dynamic_cast<IR::Declaration *>(id));
    EXPECT_EQ(id->to<IR::NamedExpression>(), dynamic_cast<IR::NamedExpression *>(id));
    EXPECT_EQ(id->to<IR::NamedExpression>(), decl);
}

TEST(RTTI, JsonRestore) {
    auto *c = new IR::Constant(2);
    IR::Expression *e1 = new IR::Add(Util::SourceInfo(), c, c);

    std::stringstream ss;
    JSONGenerator(ss) << e1 << '\n';

    JSONLoader loader(ss);

    const IR::Node *e2 = nullptr;
    loader >> e2;

    EXPECT_EQ(e2->typeId(), IR::NodeKind::Add);
}

}  // namespace Test
