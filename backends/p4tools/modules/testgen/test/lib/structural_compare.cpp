/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "backends/p4tools/common/lib/model.h"
#include "ir/compare.h"
#include "ir/ir.h"

#include "backends/p4tools/modules/testgen/lib/concolic.h"

namespace P4::P4Tools::Test {

TEST(TestgenStructuralCompare, SymbolicLabelComparatorsAgree) {
    const IR::Vector<IR::Argument> arguments;
    const IR::ConcolicVariable concolic(IR::Type_Bits::get(8), "method"_cs, &arguments, 1, 2);
    const IR::SymbolicVariable symbolic(IR::Type_Bits::get(16), concolic.label);
    EXPECT_TRUE(IR::SymbolicVariableEqual{}(symbolic, concolic));
    EXPECT_FALSE(IR::SymbolicVariableLess{}(symbolic, concolic));
    EXPECT_FALSE(IR::SymbolicVariableLess{}(concolic, symbolic));
}

TEST(TestgenStructuralCompare, ConcolicMapUsesExpressionContents) {
    const IR::Member original(new IR::PathExpression("hdr"), "field");
    const IR::Member copy(new IR::PathExpression("hdr"), "field");
    const auto *value = new IR::Constant(1);
    const auto *replacement = new IR::Constant(2);
    P4Testgen::ConcolicVariableMap values;
    values[original] = value;
    values[copy] = replacement;
    EXPECT_EQ(values.size(), 1U);
    EXPECT_EQ(values.at(original), replacement);
}

TEST(TestgenStructuralCompare, StateVariableTypesAreIgnored) {
    auto *firstRef = new IR::PathExpression("field");
    auto *secondRef = new IR::PathExpression("field");
    firstRef->type = IR::Type_Bits::get(8);
    secondRef->type = IR::Type_Bits::get(16);
    const IR::StateVariable first(firstRef), second(secondRef);
    SymbolicMapType values;
    const auto *replacement = new IR::Constant(2);
    values[first] = new IR::Constant(1);
    values[second] = replacement;
    EXPECT_EQ(values.size(), 1U);
    EXPECT_EQ(values.at(first), replacement);
}

TEST(TestgenStructuralCompare, ExtractedSizeIsMetadata) {
    IR::Extracted_Varbits a(8, false, 1), b(8, false, 2);
    ASSERT_TRUE(a.equiv(b));
    EXPECT_NE(a.structuralCompare(b), std::weak_ordering::less);
    EXPECT_NE(b.structuralCompare(a), std::weak_ordering::less);
}

}  // namespace P4::P4Tools::Test
