/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ir/annotations.h"

#include <gtest/gtest.h>

#include "ir/ir.h"

namespace P4::Test {

using namespace P4::literals;

TEST(Annotations, EmptyLists) {
    IR::ParserState state("start"_cs, nullptr);
    state.addAnnotation("add"_cs, {});
    state.addAnnotationIfNew("ifnew"_cs, {});
    state.addOrReplaceAnnotation("replace"_cs, {});

    ASSERT_EQ(state.getAnnotations().size(), 3U);
    for (const auto *annotation : state.getAnnotations()) {
        EXPECT_FALSE(annotation->needsParsing());
        EXPECT_FALSE(annotation->structured);
        EXPECT_TRUE(annotation->getExpr().empty());
    }
}

TEST(Annotations, ExpressionLists) {
    auto *first = new IR::Constant(1);
    auto *second = new IR::Constant(2);
    IR::ParserState state("start"_cs, nullptr);
    state.addAnnotation("add"_cs, {first, second}, true);
    state.addAnnotationIfNew("ifnew"_cs, {first, second}, true);
    state.addOrReplaceAnnotation("replace"_cs, {first, second}, true);

    ASSERT_EQ(state.getAnnotations().size(), 3U);
    for (const auto *annotation : state.getAnnotations()) {
        EXPECT_TRUE(annotation->structured);
        ASSERT_EQ(annotation->getExpr().size(), 2U);
        EXPECT_EQ(annotation->getExpr(0), first);
        EXPECT_EQ(annotation->getExpr(1), second);
    }
}

TEST(Annotations, SingleExpressions) {
    auto *expr = new IR::Constant(1);
    IR::ParserState state("start"_cs, nullptr);
    state.addAnnotation("add"_cs, expr);
    state.addAnnotationIfNew("ifnew"_cs, expr);
    state.addOrReplaceAnnotation("replace"_cs, expr);
    state.addAnnotation("list"_cs, {expr});

    ASSERT_EQ(state.getAnnotations().size(), 4U);
    for (const auto *annotation : state.getAnnotations()) {
        ASSERT_EQ(annotation->getExpr().size(), 1U);
        EXPECT_EQ(annotation->getExpr(0), expr);
    }
}

TEST(Annotations, PreserveAndReplace) {
    auto *expr = new IR::Constant(1);
    IR::ParserState state("start"_cs, nullptr);
    state.addAnnotation("tag"_cs, expr);
    const auto *original = state.getAnnotation("tag"_cs);
    state.addAnnotationIfNew("tag"_cs, {}, true);
    ASSERT_EQ(state.getAnnotations().size(), 1U);
    EXPECT_EQ(state.getAnnotation("tag"_cs), original);

    state.addAnnotation("tag"_cs, {expr});
    state.addAnnotation("other"_cs, {});
    const auto *other = state.getAnnotation("other"_cs);
    state.addOrReplaceAnnotation("tag"_cs, {}, true);
    ASSERT_EQ(state.getAnnotations().size(), 2U);
    EXPECT_EQ(state.getAnnotation("other"_cs), other);
    const auto *replacement = state.getAnnotation("tag"_cs);
    ASSERT_NE(replacement, nullptr);
    EXPECT_TRUE(replacement->structured);
    EXPECT_TRUE(replacement->getExpr().empty());
}

TEST(Annotations, VectorHelpers) {
    IR::Vector<IR::Annotation> annotations;
    IR::Annotations::addIfNew(annotations, "tag"_cs, {});
    ASSERT_EQ(annotations.size(), 1U);
    EXPECT_TRUE(annotations.front()->getExpr().empty());
    const auto *original = annotations.front();
    auto *expr = new IR::Constant(1);
    IR::Annotations::addIfNew(annotations, "tag"_cs, {expr}, true);
    ASSERT_EQ(annotations.size(), 1U);
    EXPECT_EQ(annotations.front(), original);

    IR::Annotations::addOrReplace(annotations, "tag"_cs, {expr}, true);
    ASSERT_EQ(annotations.size(), 1U);
    EXPECT_TRUE(annotations.front()->structured);
    ASSERT_EQ(annotations.front()->getExpr().size(), 1U);
    EXPECT_EQ(annotations.front()->getExpr(0), expr);

    IR::Annotations::addOrReplace(annotations, "tag"_cs, {});
    ASSERT_EQ(annotations.size(), 1U);
    EXPECT_TRUE(annotations.front()->getExpr().empty());
    EXPECT_FALSE(annotations.front()->structured);
}

}  // namespace P4::Test
