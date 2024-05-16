// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
//
// SPDX-License-Identifier: Apache-2.0

#include "backends/p4tools/common/lib/ir_compare.h"

#include <gtest/gtest.h>

#include <unordered_set>
#include <vector>

#include "test/gtest/helpers.h"

namespace P4::P4Tools::Test {
namespace {

struct StateVariableTest : P4::Test::P4CTest {};

TEST_F(StateVariableTest, HashAndEqualityIgnoreTypesAndIndexRepresentation) {
    const auto *bits8 = IR::Type_Bits::get(8);
    const auto *bits128 = IR::Type_Bits::get(128);
    const auto *path = new IR::PathExpression(bits8, new IR::Path("headers"));
    const auto *aliasPath = new IR::PathExpression(bits128, new IR::Path("headers"));

    // Cover plain paths, members, arrays, and members of nested arrays. Indices deliberately
    // differ in width, signedness, and printing base but denote the same arbitrary-precision value.
    for (const big_int &index : std::vector<big_int>{0, -1, (big_int(1) << 80) + 7}) {
        const auto *array = new IR::ArrayIndex(path, new IR::Constant(index, 10));
        const auto *aliasArray = new IR::ArrayIndex(
            aliasPath, new IR::Constant(IR::Type_Bits::get(128, true), index, 16));
        const std::vector<IR::StateVariable> variables{
            IR::StateVariable(path), IR::StateVariable(new IR::Member(bits8, path, "field")),
            IR::StateVariable(array),
            IR::StateVariable(new IR::Member(
                new IR::ArrayIndex(new IR::Member(array, "stack"), new IR::Constant(2)), "field"))};
        const std::vector<IR::StateVariable> aliases{
            IR::StateVariable(aliasPath),
            IR::StateVariable(new IR::Member(bits128, aliasPath, "field")),
            IR::StateVariable(aliasArray),
            IR::StateVariable(new IR::Member(new IR::ArrayIndex(new IR::Member(aliasArray, "stack"),
                                                                new IR::Constant(bits8, 2, 16)),
                                             "field"))};
        const IR::StateVariableHash hash;
        std::unordered_set<IR::StateVariable, IR::StateVariableHash, IR::StateVariableEqual> values;
        std::unordered_set<const IR::StateVariable *, IR::StateVariableHash, IR::StateVariableEqual>
            pointers;
        for (size_t i = 0; i < variables.size(); ++i) {
            EXPECT_TRUE(IR::StateVariableEqual{}(variables[i], aliases[i]));
            EXPECT_FALSE(IR::StateVariableLess{}(variables[i], aliases[i]));
            EXPECT_FALSE(IR::StateVariableLess{}(aliases[i], variables[i]));
            EXPECT_EQ(hash(variables[i]), hash(aliases[i]));
            EXPECT_EQ(hash(variables[i]), hash(&variables[i]));
            values.insert(variables[i]);
            pointers.insert(&variables[i]);
            EXPECT_EQ(values.count(aliases[i]), 1U);
            EXPECT_EQ(pointers.count(&aliases[i]), 1U);
            EXPECT_FALSE(values.insert(aliases[i]).second);
        }
        EXPECT_EQ(values.size(), variables.size());
    }
}

TEST_F(StateVariableTest, DistinctReferencesAndOrdering) {
    const auto *a = new IR::PathExpression("a");
    const auto *b = new IR::PathExpression("b");
    // This sequence captures the existing ArrayIndex < PathExpression < Member ordering,
    // including recursive base comparisons and numeric (rather than textual) index order.
    const std::vector<IR::StateVariable> variables{
        IR::StateVariable(new IR::ArrayIndex(a, new IR::Constant(2))),
        IR::StateVariable(new IR::ArrayIndex(a, new IR::Constant(10))),
        IR::StateVariable(new IR::ArrayIndex(b, new IR::Constant(0))),
        IR::StateVariable(a),
        IR::StateVariable(b),
        IR::StateVariable(new IR::Member(a, "a")),
        IR::StateVariable(new IR::Member(a, "b")),
        IR::StateVariable(new IR::Member(b, "a"))};
    std::unordered_set<IR::StateVariable, IR::StateVariableHash, IR::StateVariableEqual> values;
    for (size_t i = 0; i < variables.size(); ++i) {
        EXPECT_TRUE(values.insert(variables[i]).second);
        for (size_t j = 0; j < variables.size(); ++j) {
            EXPECT_EQ(IR::StateVariableEqual{}(variables[i], variables[j]), i == j);
            EXPECT_EQ(IR::StateVariableLess{}(variables[i], variables[j]), i < j);
        }
    }
    EXPECT_EQ(values.size(), variables.size());
}

}  // namespace
}  // namespace P4::P4Tools::Test
