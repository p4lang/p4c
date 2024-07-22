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
#include "lib/rtti_utils.h"

namespace P4C::Test {

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

TEST(RttiUtils, to) {
    const auto *c = IR::Constant::get(IR::Type::Bits::get(4), 2);
    const IR::Node *n = c;
    const IR::Node *nullNode = nullptr;

    EXPECT_EQ(RTTI::to<IR::Constant>(n), c);
    EXPECT_EQ(RTTI::to<IR::Constant>(c), c);
    EXPECT_EQ(RTTI::to<IR::Add>(c), nullptr);
    EXPECT_EQ(RTTI::to<IR::Add>(nullNode), nullptr);

    EXPECT_EQ(RTTI::to<IR::Literal>(n), n->to<IR::Literal>());
    EXPECT_EQ(RTTI::to<IR::Type_Boolean>(c->type), nullptr);

    std::vector<const IR::Node *> from{c, new IR::Add(c, c), n, new IR::LNot(c)};
    std::vector<const IR::Operation *> to;
    std::transform(from.begin(), from.end(), std::back_inserter(to), RTTI::to<IR::Operation>);

    ASSERT_EQ(to.size(), 4);
    EXPECT_EQ(to[0], nullptr);
    ASSERT_NE(to[1], nullptr);
    EXPECT_TRUE(to[1]->is<IR::Add>());
    EXPECT_EQ(to[2], nullptr);
    ASSERT_NE(to[3], nullptr);
    EXPECT_TRUE(to[3]->is<IR::LNot>());
}

TEST(RttiUtils, is) {
    const auto *c = IR::Constant::get(IR::Type::Bits::get(4), 2);
    const IR::Node *n = c;
    const IR::Node *nullNode = nullptr;

    EXPECT_TRUE(RTTI::is<IR::Constant>(n));
    EXPECT_TRUE(RTTI::is<IR::Constant>(c));
    EXPECT_FALSE(RTTI::is<IR::Add>(c));
    EXPECT_FALSE(RTTI::is<IR::BoolLiteral>(c));
    EXPECT_FALSE(RTTI::is<IR::Add>(nullNode));

    std::vector<const IR::Node *> from{c, new IR::Add(c, c), n, new IR::LNot(c)};
    auto it = std::find_if(from.begin(), from.end(), RTTI::is<IR::Operation_Unary>);

    EXPECT_NE(it, from.end());
    EXPECT_EQ(it, std::prev(from.end()));
    EXPECT_EQ(*it, from[3]);
}

TEST(RttiUtils, isAny) {
    const auto *c = IR::Constant::get(IR::Type::Bits::get(4), 2);
    const IR::Node *n = c;
    const IR::Node *nullNode = nullptr;

    EXPECT_TRUE(RTTI::isAny<IR::Constant>(n));
    EXPECT_TRUE(RTTI::isAny<IR::Constant>(c));
    EXPECT_FALSE(RTTI::isAny<IR::Add>(c));
    EXPECT_FALSE(RTTI::isAny<IR::BoolLiteral>(c));
    EXPECT_FALSE(RTTI::isAny<IR::Add>(nullNode));

    EXPECT_TRUE((RTTI::isAny<IR::BoolLiteral, IR::Constant>(n)));
    // EXPECT_TRUE(RTTI::isAny<>(n)); // does not compile, with is right
    EXPECT_FALSE((RTTI::isAny<IR::Add, IR::LOr, IR::BAnd>(c)));

    std::vector<const IR::Node *> from{c, new IR::Add(c, c), n, new IR::LNot(c)};
    auto it = std::find_if(from.begin(), from.end(),
                           RTTI::isAny<IR::Operation_Unary, IR::Operation_Binary>);

    EXPECT_NE(it, from.end());
    EXPECT_EQ(it, std::next(from.begin()));
    EXPECT_EQ(*it, from[1]);
}

}  // namespace P4C::Test
