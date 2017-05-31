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

TEST(IR, OperatorEq) {
    auto *t = IR::Type::Bits::get(16);
    IR::Constant *a = new IR::Constant(t, 10);
    IR::Constant *b = new IR::Constant(t, 10);
    IR::Constant *c = new IR::Constant(t, 20);

    EXPECT_EQ(*a, *b);
    EXPECT_EQ(*a, *static_cast<IR::Expression *>(b));
    EXPECT_EQ(*a, *static_cast<IR::Node *>(b));
    EXPECT_EQ(*static_cast<IR::Expression *>(a), *b);
    EXPECT_EQ(*static_cast<IR::Expression *>(a), *static_cast<IR::Expression *>(b));
    EXPECT_EQ(*static_cast<IR::Expression *>(a), *static_cast<IR::Node *>(b));
    EXPECT_EQ(*static_cast<IR::Node *>(a), *b);
    EXPECT_EQ(*static_cast<IR::Node *>(a), *static_cast<IR::Expression *>(b));
    EXPECT_EQ(*static_cast<IR::Node *>(a), *static_cast<IR::Node *>(b));

    EXPECT_NE(*a, *c);
    EXPECT_NE(*a, *static_cast<IR::Expression *>(c));
    EXPECT_NE(*a, *static_cast<IR::Node *>(c));
    EXPECT_NE(*static_cast<IR::Expression *>(a), *c);
    EXPECT_NE(*static_cast<IR::Expression *>(a), *static_cast<IR::Expression *>(c));
    EXPECT_NE(*static_cast<IR::Expression *>(a), *static_cast<IR::Node *>(c));
    EXPECT_NE(*static_cast<IR::Node *>(a), *c);
    EXPECT_NE(*static_cast<IR::Node *>(a), *static_cast<IR::Expression *>(c));
    EXPECT_NE(*static_cast<IR::Node *>(a), *static_cast<IR::Node *>(c));

    auto *p1 = new IR::IndexedVector<IR::Node>(a);
    auto *p2 = p1->clone();
    auto *p3 = new IR::IndexedVector<IR::Node>(c);

    EXPECT_EQ(*p1, *p2);
    EXPECT_EQ(*p1, *static_cast<IR::Vector<IR::Node> *>(p2));
    EXPECT_EQ(*p1, *static_cast<IR::Node *>(p2));
    EXPECT_EQ(*static_cast<IR::Vector<IR::Node> *>(p1), *p2);
    EXPECT_EQ(*static_cast<IR::Vector<IR::Node> *>(p1), *static_cast<IR::Vector<IR::Node> *>(p2));
    EXPECT_EQ(*static_cast<IR::Vector<IR::Node> *>(p1), *static_cast<IR::Node *>(p2));
    EXPECT_EQ(*static_cast<IR::Node *>(p1), *p2);
    EXPECT_EQ(*static_cast<IR::Node *>(p1), *static_cast<IR::Vector<IR::Node> *>(p2));
    EXPECT_EQ(*static_cast<IR::Node *>(p1), *static_cast<IR::Node *>(p2));

    EXPECT_NE(*p1, *p3);
    EXPECT_NE(*p1, *static_cast<IR::Vector<IR::Node> *>(p3));
    EXPECT_NE(*p1, *static_cast<IR::Node *>(p3));
    EXPECT_NE(*static_cast<IR::Vector<IR::Node> *>(p1), *p3);
    EXPECT_NE(*static_cast<IR::Vector<IR::Node> *>(p1), *static_cast<IR::Vector<IR::Node> *>(p3));
    EXPECT_NE(*static_cast<IR::Vector<IR::Node> *>(p1), *static_cast<IR::Node *>(p3));
    EXPECT_NE(*static_cast<IR::Node *>(p1), *p3);
    EXPECT_NE(*static_cast<IR::Node *>(p1), *static_cast<IR::Vector<IR::Node> *>(p3));
    EXPECT_NE(*static_cast<IR::Node *>(p1), *static_cast<IR::Node *>(p3));
}
