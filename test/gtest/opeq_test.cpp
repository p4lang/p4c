// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "ir/core.h"
#include "ir/visitor.h"
#include "lib/exceptions.h"

using namespace P4;

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

TEST(IR, ContainerEqualityDispatch) {
    auto *value = new IR::Constant(1);
    IR::Vector<IR::Node> nodes{value};
    IR::Vector<IR::Node> sameNodes{value};
    IR::Vector<IR::Expression> expressions{value};
    IR::IndexedVector<IR::Node> indexed{value};
    IR::IndexedVector<IR::Node> sameIndexed{value};
    const IR::Node &node = nodes;
    const IR::Node &expression = expressions;
    const IR::Node &index = indexed;

    EXPECT_EQ(node, sameNodes);
    EXPECT_EQ(nodes, static_cast<const IR::Node &>(sameNodes));
    EXPECT_EQ(index, sameIndexed);
    EXPECT_EQ(indexed, static_cast<const IR::Node &>(sameIndexed));
    EXPECT_NE(node, expression);
    EXPECT_NE(expression, node);
    EXPECT_NE(node, index);
    EXPECT_NE(index, node);
    EXPECT_NE(node, *value);
    EXPECT_NE(*value, node);
}
