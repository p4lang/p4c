/*
Copyright 2024-present New York University.

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

#include "ir/structural_compare.h"

#include <gtest/gtest.h>

#include <map>
#include <optional>
#include <set>
#include <tuple>
#include <variant>
#include <vector>

#include "frontends/common/parseInput.h"
#include "helpers.h"
#include "ir/compare.h"
#include "ir/ir.h"
#include "ir/node.h"

using namespace P4::literals;

namespace P4::Test {

bool structuralLess(const IR::Node &a, const IR::Node &b) { return a.structuralCompare(b) < 0; }

#define EXECUTE_FUNCTION_FOR_P4C_NODE(BASE_TYPE, a, b, function)                                   \
    EXPECT_TRUE(function(*(a), *(b)));                                                             \
    EXPECT_TRUE(function(*(a), *static_cast<const BASE_TYPE *>(b)));                               \
    EXPECT_TRUE(function(*(a), *static_cast<const IR::Node *>(b)));                                \
    EXPECT_TRUE(function(*static_cast<const BASE_TYPE *>(a), *(b)));                               \
    EXPECT_TRUE(function(*static_cast<const BASE_TYPE *>(a), *static_cast<const BASE_TYPE *>(b))); \
    EXPECT_TRUE(function(*static_cast<const BASE_TYPE *>(a), *static_cast<const IR::Node *>(b)));  \
    EXPECT_TRUE(function(*static_cast<const IR::Node *>(a), *(b)));                                \
    EXPECT_TRUE(function(*static_cast<const IR::Node *>(a), *static_cast<const BASE_TYPE *>(b)));  \
    EXPECT_TRUE(function(*static_cast<const IR::Node *>(a), *static_cast<const IR::Node *>(b)));

#define CHECK_LESS_FOR_P4C_NODE(BASE_TYPE, a, b) \
    EXECUTE_FUNCTION_FOR_P4C_NODE(BASE_TYPE, a, b, structuralLess)
#define CHECK_LESS_FOR_P4C_TYPE(a, b) CHECK_LESS_FOR_P4C_NODE(IR::Type, a, b)
#define CHECK_LESS_FOR_P4C_EXPRESSION(a, b) CHECK_LESS_FOR_P4C_NODE(IR::Expression, a, b)
#define CHECK_LESS_FOR_P4C_NODE_VECTOR(a, b) CHECK_LESS_FOR_P4C_NODE(IR::Vector<IR::Node>, a, b)
#define CHECK_LESS_FOR_P4C_DECLARATION(a, b) CHECK_LESS_FOR_P4C_NODE(IR::Declaration, a, b)

#define CHECK_GREATER_FOR_P4C_TYPE(a, b) CHECK_LESS_FOR_P4C_TYPE(b, a)
#define CHECK_GREATER_FOR_P4C_EXPRESSION(a, b) CHECK_LESS_FOR_P4C_EXPRESSION(b, a)
#define CHECK_GREATER_FOR_P4C_NODE_VECTOR(a, b) CHECK_LESS_FOR_P4C_NODE_VECTOR(b, a)
#define CHECK_GREATER_FOR_P4C_DECLARATION(a, b) CHECK_LESS_FOR_P4C_DECLARATION(b, a)

bool checkEqualityWithLess(const IR::Node &a, const IR::Node &b) {
    return a.structuralCompare(b) == 0;
}

#define CHECK_EQUALITY_WITH_LESS(BASE_TYPE, a, b) \
    EXECUTE_FUNCTION_FOR_P4C_NODE(BASE_TYPE, a, b, checkEqualityWithLess)
#define CHECK_EQUALITY_FOR_P4C_TYPE(a, b) CHECK_EQUALITY_WITH_LESS(IR::Type, a, b)
#define CHECK_EQUALITY_FOR_P4C_EXPRESSION(a, b) CHECK_EQUALITY_WITH_LESS(IR::Expression, a, b)
#define CHECK_EQUALITY_FOR_P4C_NODE_VECTOR(a, b) \
    CHECK_EQUALITY_WITH_LESS(IR::Vector<IR::Node>, a, b)
#define CHECK_EQUALITY_FOR_P4C_DECLARATION(a, b) CHECK_EQUALITY_WITH_LESS(IR::Declaration, a, b)

TEST(StructuralCompare, Types) {
    const auto *a = IR::Type_Bits::get(16, false);
    const auto *b = IR::Type_Bits::get(32, false);
    const auto *c = IR::Type_Bits::get(16, false);

    CHECK_LESS_FOR_P4C_TYPE(a, b);
    CHECK_GREATER_FOR_P4C_TYPE(b, a);
    CHECK_EQUALITY_FOR_P4C_TYPE(a, c);

    const auto *d = IR::Type_Boolean::get();
    CHECK_LESS_FOR_P4C_TYPE(d, a);
    CHECK_GREATER_FOR_P4C_TYPE(a, d);

    const auto *e = IR::Type_InfInt::get();
    CHECK_LESS_FOR_P4C_TYPE(a, e);
    CHECK_GREATER_FOR_P4C_TYPE(e, a);

    const auto *f = new IR::Type_Name("f");
    const auto *f2 = new IR::Type_Name("f");
    CHECK_LESS_FOR_P4C_TYPE(a, f);
    CHECK_GREATER_FOR_P4C_TYPE(f, a);
    CHECK_EQUALITY_FOR_P4C_TYPE(f, f2)

    const auto *g = IR::Type_String::get();
    CHECK_LESS_FOR_P4C_TYPE(a, g);
    CHECK_GREATER_FOR_P4C_TYPE(g, a);
}

TEST(StructuralCompare, Constants) {
    // Check unsigned constants.
    {
        const auto *a = IR::Constant::get(IR::Type_Bits::get(16, false), 5);
        const auto *b = IR::Constant::get(IR::Type_Bits::get(16, false), 10);
        const auto *c = IR::Constant::get(IR::Type_Bits::get(16, false), 5);

        CHECK_LESS_FOR_P4C_EXPRESSION(a, b);
        CHECK_GREATER_FOR_P4C_EXPRESSION(b, a);
        CHECK_EQUALITY_FOR_P4C_EXPRESSION(a, c);
    }
    // Check signed constants.
    {
        const auto *a = IR::Constant::get(IR::Type_Bits::get(16, true), -1);
        const auto *b = IR::Constant::get(IR::Type_Bits::get(16, true), 0);
        const auto *c = IR::Constant::get(IR::Type_Bits::get(16, true), -1);

        CHECK_LESS_FOR_P4C_EXPRESSION(a, b);
        CHECK_EQUALITY_FOR_P4C_EXPRESSION(a, c);
    }
    // Check Strings
    {
        const auto *a = IR::StringLiteral::get("a"_cs, IR::Type_String::get());
        const auto *b = IR::StringLiteral::get("b"_cs, IR::Type_String::get());
        const auto *a2 = IR::StringLiteral::get("a"_cs, IR::Type_String::get());

        CHECK_LESS_FOR_P4C_EXPRESSION(a, b);
        CHECK_GREATER_FOR_P4C_EXPRESSION(b, a);
        CHECK_EQUALITY_FOR_P4C_EXPRESSION(a, a2);
    }
    // Check booleans
    {
        const auto *a = IR::BoolLiteral::get(false);
        const auto *b = IR::BoolLiteral::get(true);
        const auto *c = IR::BoolLiteral::get(false);

        CHECK_LESS_FOR_P4C_EXPRESSION(a, b);
        CHECK_GREATER_FOR_P4C_EXPRESSION(b, a);
        CHECK_EQUALITY_FOR_P4C_EXPRESSION(a, c);
    }
    // Check infinite precision integers.
    {
        const auto *a = IR::Constant::get(IR::Type_InfInt::get(), 5);
        const auto *b = IR::Constant::get(IR::Type_InfInt::get(), 10);
        const auto *c = IR::Constant::get(IR::Type_InfInt::get(), 5);

        CHECK_LESS_FOR_P4C_EXPRESSION(a, b);
        CHECK_GREATER_FOR_P4C_EXPRESSION(b, a);
        CHECK_EQUALITY_FOR_P4C_EXPRESSION(a, c);
    }
}

TEST(StructuralCompare, MixedConstants) {
    const auto *t = IR::Type_Bits::get(16, false);
    const auto *a = IR::StringLiteral::get("a"_cs, IR::Type_String::get());
    const auto *b = IR::Constant::get(t, 0);
    const auto *c = IR::Constant::get(t, 10);
    const auto *d = IR::BoolLiteral::get(false);

    CHECK_LESS_FOR_P4C_EXPRESSION(b, a);
    CHECK_LESS_FOR_P4C_EXPRESSION(c, a);
    CHECK_GREATER_FOR_P4C_EXPRESSION(a, b);
    CHECK_GREATER_FOR_P4C_EXPRESSION(a, c);

    CHECK_LESS_FOR_P4C_EXPRESSION(d, a);
    CHECK_GREATER_FOR_P4C_EXPRESSION(a, d);
    CHECK_LESS_FOR_P4C_EXPRESSION(b, d);
    CHECK_GREATER_FOR_P4C_EXPRESSION(d, b);
}

TEST(StructuralCompare, ConstantVectors) {
    // Two empty vectors are equal.
    auto *e1 = new IR::IndexedVector<IR::Node>();
    auto *e2 = new IR::IndexedVector<IR::Node>();
    CHECK_EQUALITY_FOR_P4C_NODE_VECTOR(e1, e2);

    const auto *t = IR::Type_Bits::get(16, false);
    const auto *a = IR::Constant::get(t, 5);
    const auto *b = IR::Constant::get(t, 10);
    const auto *c = IR::Constant::get(t, 5);

    auto *p1 = new IR::IndexedVector<IR::Node>(a);
    auto *p2 = new IR::IndexedVector<IR::Node>(b);
    auto *p3 = new IR::IndexedVector<IR::Node>(c);

    CHECK_LESS_FOR_P4C_NODE_VECTOR(p1, p2);
    CHECK_GREATER_FOR_P4C_NODE_VECTOR(p2, p1);
    CHECK_EQUALITY_FOR_P4C_NODE_VECTOR(p1, p3);

    // Check that we correctly compare uneven vectors.
    p2->push_back(b);
    CHECK_LESS_FOR_P4C_NODE_VECTOR(p1, p2);
    CHECK_GREATER_FOR_P4C_NODE_VECTOR(p2, p1);
    p1->push_back(b);
    p1->push_back(b);
    CHECK_LESS_FOR_P4C_NODE_VECTOR(p1, p2);
    CHECK_GREATER_FOR_P4C_NODE_VECTOR(p2, p1);

    // Check that we correctly compare vectors with different nodes.
    auto *p4 = new IR::IndexedVector<IR::Node>(IR::BoolLiteral::get(false));
    auto *p5 = new IR::IndexedVector<IR::Node>(new IR::Constant(t, 0));
    auto *p6 =
        new IR::IndexedVector<IR::Node>(IR::StringLiteral::get("a"_cs, IR::Type_String::get()));

    CHECK_LESS_FOR_P4C_NODE_VECTOR(p5, p4);
    CHECK_GREATER_FOR_P4C_NODE_VECTOR(p4, p5);
    CHECK_LESS_FOR_P4C_NODE_VECTOR(p4, p6);
    CHECK_GREATER_FOR_P4C_NODE_VECTOR(p6, p4);
    CHECK_LESS_FOR_P4C_NODE_VECTOR(p5, p6);
    CHECK_GREATER_FOR_P4C_NODE_VECTOR(p6, p5);
}

TEST(StructuralCompare, UnaryExpressions) {
    const auto *t = IR::Type_Bits::get(16, false);
    const auto *a = IR::Constant::get(t, 5);
    const auto *b = IR::Constant::get(t, 10);
    const auto *c = IR::Constant::get(t, 5);

    const auto *p1 = new IR::LNot(a);
    const auto *p2 = new IR::LNot(b);
    const auto *p3 = new IR::LNot(c);

    CHECK_LESS_FOR_P4C_EXPRESSION(p1, p2);
    CHECK_GREATER_FOR_P4C_EXPRESSION(p2, p1);
    CHECK_EQUALITY_FOR_P4C_EXPRESSION(p1, p3);
}

TEST(StructuralCompare, BinaryExpressions) {
    const auto *t = IR::Type_Bits::get(16, false);
    const auto *a = IR::Constant::get(t, 5);
    const auto *b = IR::Constant::get(t, 10);
    const auto *c = IR::Constant::get(t, 5);

    const auto *p1 = new IR::Add(a, b);
    const auto *p2 = new IR::Add(b, a);
    const auto *p3 = new IR::Add(c, b);

    CHECK_LESS_FOR_P4C_EXPRESSION(p1, p2);
    CHECK_GREATER_FOR_P4C_EXPRESSION(p2, p1);
    CHECK_EQUALITY_FOR_P4C_EXPRESSION(p1, p3);
}

TEST(StructuralCompare, TernaryExpressions) {
    const auto *t = IR::Type_Bits::get(16, false);
    const auto *a = IR::Constant::get(t, 5);
    const auto *b = IR::Constant::get(t, 10);
    const auto *c = IR::Constant::get(t, 5);

    const auto *trueCond = IR::BoolLiteral::get(true);
    const auto *falseCond = IR::BoolLiteral::get(false);

    const auto *p1 = new IR::Mux(trueCond, a, c);
    const auto *p2 = new IR::Mux(trueCond, b, c);
    const auto *p3 = new IR::Mux(trueCond, c, a);

    CHECK_LESS_FOR_P4C_EXPRESSION(p1, p2);
    CHECK_GREATER_FOR_P4C_EXPRESSION(p2, p1);
    CHECK_EQUALITY_FOR_P4C_EXPRESSION(p1, p3);

    const auto *p4 = new IR::Mux(falseCond, a, c);

    CHECK_LESS_FOR_P4C_EXPRESSION(p4, p1);
    CHECK_LESS_FOR_P4C_EXPRESSION(p4, p2);
    CHECK_GREATER_FOR_P4C_EXPRESSION(p1, p4);
    CHECK_GREATER_FOR_P4C_EXPRESSION(p2, p4);
}

TEST(StructuralCompare, PathExpressions) {
    const auto *a = new IR::PathExpression("a");
    const auto *b = new IR::PathExpression("b");
    const auto *a2 = new IR::PathExpression("a");

    CHECK_LESS_FOR_P4C_EXPRESSION(a, b);
    CHECK_GREATER_FOR_P4C_EXPRESSION(b, a);
    CHECK_EQUALITY_FOR_P4C_EXPRESSION(a, a2);

    const auto *c = new IR::PathExpression(new IR::Path("c"));
    const auto *d = new IR::PathExpression(new IR::Path("d"));
    const auto *c2 = new IR::PathExpression(new IR::Path("c"));

    CHECK_LESS_FOR_P4C_EXPRESSION(c, d);
    CHECK_GREATER_FOR_P4C_EXPRESSION(d, c);
    CHECK_EQUALITY_FOR_P4C_EXPRESSION(c, c2);
}

TEST(StructuralCompare, Members) {
    const auto *a = new IR::Member(new IR::PathExpression("a"), "b");
    const auto *b = new IR::Member(new IR::PathExpression("a"), "c");
    const auto *a2 = new IR::Member(new IR::PathExpression("a"), "b");

    CHECK_LESS_FOR_P4C_EXPRESSION(a, b);
    CHECK_GREATER_FOR_P4C_EXPRESSION(b, a);
    CHECK_EQUALITY_FOR_P4C_EXPRESSION(a, a2);

    const auto *c = new IR::Member(new IR::PathExpression("b"), "a");
    CHECK_LESS_FOR_P4C_EXPRESSION(a, c);
    CHECK_GREATER_FOR_P4C_EXPRESSION(c, a);
}

TEST(StructuralCompare, Declarations) {
    const auto *t = IR::Type_Bits::get(16, false);
    {
        const auto *a = new IR::Declaration_Variable("a", t);
        const auto *b = new IR::Declaration_Variable("b", t);
        const auto *a2 = new IR::Declaration_Variable("a", t);

        CHECK_LESS_FOR_P4C_DECLARATION(a, b);
        CHECK_GREATER_FOR_P4C_DECLARATION(b, a);
        CHECK_EQUALITY_FOR_P4C_DECLARATION(a, a2);
    }

    {
        const auto *a = new IR::Declaration_Constant("a", t, IR::Constant::get(t, 0));
        const auto *b = new IR::Declaration_Constant("b", t, IR::Constant::get(t, 0));
        const auto *a2 = new IR::Declaration_Constant("a", t, IR::Constant::get(t, 0));
        CHECK_LESS_FOR_P4C_DECLARATION(a, b);
        CHECK_GREATER_FOR_P4C_DECLARATION(b, a);
        CHECK_EQUALITY_FOR_P4C_DECLARATION(a, a2);

        const auto *a3 = new IR::Declaration_Constant("a", t, IR::Constant::get(t, 1));
        CHECK_LESS_FOR_P4C_DECLARATION(a, a3);
        CHECK_GREATER_FOR_P4C_DECLARATION(a3, a);
    }
}

TEST(StructuralCompare, FirstDifferenceWins) {
    // An earlier, greater field must prevent a later, smaller field from winning.
    const IR::Mux a(IR::BoolLiteral::get(true), new IR::Constant(1), new IR::Constant(0));
    const IR::Mux b(IR::BoolLiteral::get(false), new IR::Constant(1), new IR::Constant(2));
    EXPECT_NE(a.structuralCompare(b), std::weak_ordering::less);
    EXPECT_EQ(b.structuralCompare(a), std::weak_ordering::less);

    // The same rule applies between inherited and local fields.
    const IR::Constant narrow(IR::Type_Bits::get(8), 10);
    const IR::Constant wide(IR::Type_Bits::get(16), 0);
    EXPECT_EQ(narrow.structuralCompare(wide), std::weak_ordering::less);
    EXPECT_NE(wide.structuralCompare(narrow), std::weak_ordering::less);
    const IR::Declaration_Variable first("a", IR::Type_Bits::get(16));
    const IR::Declaration_Variable second("b", IR::Type_Bits::get(8));
    EXPECT_EQ(first.structuralCompare(second), std::weak_ordering::less);
    EXPECT_NE(second.structuralCompare(first), std::weak_ordering::less);
}

TEST(StructuralCompare, ConstantDisplayBase) {
    const IR::Constant decimal(IR::Type_Bits::get(8), 42, 10);
    const IR::Constant hexadecimal(IR::Type_Bits::get(8), 42, 16);
    EXPECT_EQ(decimal.structuralCompare(hexadecimal), std::weak_ordering::equivalent);
    EXPECT_FALSE(decimal.equiv(hexadecimal));  // Display base participates in equiv.
}

TEST(StructuralCompare, TypeDeclarations) {
    const IR::Type_Typedef a("a", IR::Type_Bits::get(16));
    const IR::Type_Typedef b("b", IR::Type_Bits::get(8));
    const IR::Type_Typedef copy("a", IR::Type_Bits::get(16));
    EXPECT_EQ(a.structuralCompare(b), std::weak_ordering::less);
    EXPECT_NE(b.structuralCompare(a), std::weak_ordering::less);
    EXPECT_NE(a.structuralCompare(copy), std::weak_ordering::less);
    EXPECT_NE(copy.structuralCompare(a), std::weak_ordering::less);
}

TEST(StructuralCompare, AnnotationVariants) {
    const IR::Annotation a("a", new IR::Constant(1));
    const IR::Annotation b("a", new IR::Constant(2));
    const IR::Annotation copy("a", new IR::Constant(1));
    const IR::Annotation unparsed(Util::SourceInfo(), "a", IR::Vector<IR::AnnotationToken>{});
    EXPECT_EQ(a.structuralCompare(b), std::weak_ordering::less);
    EXPECT_NE(b.structuralCompare(a), std::weak_ordering::less);
    EXPECT_NE(a.structuralCompare(copy), std::weak_ordering::less);
    EXPECT_NE(copy.structuralCompare(a), std::weak_ordering::less);
    EXPECT_EQ(unparsed.structuralCompare(a), std::weak_ordering::less);
    EXPECT_NE(a.structuralCompare(unparsed), std::weak_ordering::less);
}

TEST(StructuralCompare, NullableStrings) {
    cstring absent;
    EXPECT_EQ(IR::structuralCompare(absent, cstring::empty), std::weak_ordering::less);
    EXPECT_NE(IR::structuralCompare(cstring::empty, absent), std::weak_ordering::less);
    EXPECT_NE(IR::structuralCompare(absent, absent), std::weak_ordering::less);
    const char first[] = "abc", equal[] = "abc", second[] = "abd";
    EXPECT_EQ(IR::structuralCompare(first, equal), std::weak_ordering::equivalent);
    EXPECT_EQ(IR::structuralCompare(first, second), std::weak_ordering::less);
    EXPECT_EQ(IR::structuralCompare(second, first), std::weak_ordering::greater);
}

TEST(StructuralCompare, NullsAndContainers) {
    const IR::Expression *null = nullptr;
    const IR::Expression *a = new IR::Constant(1);
    const IR::Expression *copy = new IR::Constant(1);
    const IR::Expression *b = new IR::Constant(2);
    IR::StructuralLess less;
    EXPECT_FALSE(less(null, null));
    EXPECT_TRUE(less(null, a));
    EXPECT_FALSE(less(a, null));
    EXPECT_FALSE(less(a, copy));
    EXPECT_FALSE(less(copy, a));

    std::vector<const IR::Expression *> left{a, null}, right{copy, b};
    EXPECT_EQ(IR::structuralCompare(left, right), std::weak_ordering::less);
    EXPECT_NE(IR::structuralCompare(right, left), std::weak_ordering::less);
    const IR::Expression *leftArray[] = {a, null}, *rightArray[] = {copy, b};
    EXPECT_EQ(IR::structuralCompare(leftArray, rightArray), std::weak_ordering::less);
    EXPECT_NE(IR::structuralCompare(rightArray, leftArray), std::weak_ordering::less);
    EXPECT_EQ(IR::structuralCompare(std::tie(a, null), std::tie(copy, b)),
              std::weak_ordering::less);
    EXPECT_NE(IR::structuralCompare(std::tie(copy, b), std::tie(a, null)),
              std::weak_ordering::less);
    EXPECT_NE(IR::structuralCompare(std::tuple{}, std::tuple{}), std::weak_ordering::less);

    std::map<int, const IR::Expression *> first{{0, a}}, equal{{0, copy}}, second{{0, b}};
    EXPECT_NE(IR::structuralCompare(first, equal), std::weak_ordering::less);
    EXPECT_NE(IR::structuralCompare(equal, first), std::weak_ordering::less);
    EXPECT_EQ(IR::structuralCompare(first, second), std::weak_ordering::less);
    EXPECT_NE(IR::structuralCompare(second, first), std::weak_ordering::less);
    std::variant<int, int> v1(std::in_place_index<1>, 1), v2(std::in_place_index<1>, 2);
    EXPECT_EQ(IR::structuralCompare(v1, v2), std::weak_ordering::less);

    std::set<const IR::Node *, IR::StructuralLess> nodes{a, copy, b};
    EXPECT_EQ(nodes.size(), 2U);
}

TEST(StructuralCompare, NameMaps) {
    IR::NameMap<IR::Expression> a, b, copy;
    a.add("x"_cs, new IR::Constant(1));
    b.add("x"_cs, new IR::Constant(2));
    copy.add("x"_cs, new IR::Constant(1));
    EXPECT_EQ(a.structuralCompare(b), std::weak_ordering::less);
    EXPECT_NE(b.structuralCompare(a), std::weak_ordering::less);
    EXPECT_NE(a.structuralCompare(copy), std::weak_ordering::less);
    EXPECT_NE(copy.structuralCompare(a), std::weak_ordering::less);
}

TEST(StructuralCompare, StrictWeakOrdering) {
    // Include distinct allocations, runtime types, inherited fields and variant alternatives.
    std::vector<const IR::Node *> nodes{
        new IR::Constant(IR::Type_Bits::get(8), 10),
        new IR::Constant(IR::Type_Bits::get(16), 0),
        new IR::Constant(IR::Type_Bits::get(8), 10),
        new IR::PathExpression("a"),
        new IR::Type_Typedef("a", IR::Type_Bits::get(8)),
        new IR::Declaration_Variable("a", IR::Type_Bits::get(8)),
        new IR::Vector<IR::Node>(),
        new IR::Vector<IR::Expression>(),
        new IR::IndexedVector<IR::Node>(),
        new IR::NameMap<IR::Expression>(),
        new IR::Annotation("a", new IR::Constant(1)),
        new IR::Annotation("a", new IR::Constant(2)),
        new IR::Annotation(Util::SourceInfo(), "a", IR::Vector<IR::AnnotationToken>{})};
    IR::StructuralLess less;
    for (const auto *a : nodes) {
        EXPECT_FALSE(less(a, a));
        for (const auto *b : nodes) {
            const auto order = a->structuralCompare(*b);
            EXPECT_EQ(order < 0, b->structuralCompare(*a) > 0);
            EXPECT_EQ(order == 0, b->structuralCompare(*a) == 0);
            EXPECT_EQ(less(a, b), order < 0);
            EXPECT_FALSE(less(a, b) && less(b, a));
            for (const auto *c : nodes) {
                if (less(a, b) && less(b, c)) EXPECT_TRUE(less(a, c));
                if (!less(a, b) && !less(b, a) && !less(b, c) && !less(c, b)) {
                    EXPECT_FALSE(less(a, c));
                    EXPECT_FALSE(less(c, a));
                }
            }
        }
    }
}

TEST(StructuralCompare, EqualityIsSeparateFromNodeEquality) {
    const IR::Declaration_Variable first("x", IR::Type_Bits::get(8));
    const IR::Declaration_Variable second("x", IR::Type_Bits::get(8));
    ASSERT_NE(first.declid, second.declid);
    EXPECT_FALSE(first == second);
    EXPECT_EQ(first.structuralCompare(second), std::weak_ordering::equivalent);
    std::set<const IR::Node *, IR::StructuralLess> declarations{&first, &second};
    EXPECT_EQ(declarations.size(), 1U);
}

TEST(StructuralCompare, OptionalPointers) {
    const IR::Expression *a = new IR::Constant(1);
    const IR::Expression *copy = new IR::Constant(1);
    const std::optional<const IR::Expression *> absent, null{nullptr}, first{a}, second{copy};
    EXPECT_EQ(IR::structuralCompare(absent, null), std::weak_ordering::less);
    EXPECT_EQ(IR::structuralCompare(null, first), std::weak_ordering::less);
    EXPECT_EQ(IR::structuralCompare(first, second), std::weak_ordering::equivalent);
}

namespace {
class CountingConstant : public IR::Constant {
    size_t &comparisons;

 public:
    explicit CountingConstant(size_t &comparisons)
        : IR::Constant(IR::Type_Bits::get(8), 1), comparisons(comparisons) {}
    std::weak_ordering structuralCompare(const IR::Node &other) const override {
        ++comparisons;
        return IR::Constant::structuralCompare(other);
    }
};
}  // namespace

TEST(StructuralCompare, EqualSubtreesAreComparedOnce) {
    size_t comparisons = 0;
    const IR::Expression *first = new CountingConstant(comparisons);
    const IR::Expression *second = new CountingConstant(comparisons);
    for (int depth = 0; depth < 12; ++depth) {
        first = new IR::Neg(first);
        second = new IR::Neg(second);
    }
    EXPECT_EQ(first->structuralCompare(*second), std::weak_ordering::equivalent);
    EXPECT_EQ(comparisons, 1U);
}

TEST(StructuralCompare, StopsAtFirstDifferenceAndSharedPointers) {
    size_t comparisons = 0;
    const IR::Expression *first = new CountingConstant(comparisons);
    const IR::Expression *second = new CountingConstant(comparisons);
    const IR::Add a(new IR::Constant(0), first), b(new IR::Constant(1), second);
    EXPECT_EQ(a.structuralCompare(b), std::weak_ordering::less);
    EXPECT_EQ(comparisons, 0U);
    EXPECT_EQ(IR::structuralCompare(first, first), std::weak_ordering::equivalent);
    EXPECT_EQ(comparisons, 0U);

    const std::vector<const IR::Expression *> left{first, first}, right{second, second};
    EXPECT_EQ(IR::structuralCompare(left, right), std::weak_ordering::equivalent);
    EXPECT_EQ(comparisons, 2U);
    comparisons = 0;
    EXPECT_EQ(IR::structuralCompare(std::tie(first, first), std::tie(second, second)),
              std::weak_ordering::equivalent);
    EXPECT_EQ(comparisons, 2U);
}

TEST_F(P4CTest, StructuralOrderingParsedProgram) {
    const std::string source = R"(
@unparsed(1, "value")
@expressions[1, 2]
@fields[first=1, second=2]
typedef bit<8> Byte;

struct Metadata {
    Byte first;
    bit<16> second;
}

control StructuralOrdering(inout Metadata meta) {
    apply {
        meta.first = meta.second == 0 ? 8w1 : 8w2;
    }
}

control Stage(inout Metadata meta);
package Pipeline(Stage controlInstance);
Pipeline(StructuralOrdering()) main;
    )";
    const auto *first = P4::parseP4String(source);
    const auto *second = P4::parseP4String("\n" + source);
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    ASSERT_NE(first, second);
    EXPECT_NE(first->structuralCompare(*second), std::weak_ordering::less);
    EXPECT_NE(second->structuralCompare(*first), std::weak_ordering::less);
}

}  // namespace P4::Test
