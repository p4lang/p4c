// SPDX-FileCopyrightText: 2025 Altera Corporation
// Copyright 2025-present Altera Corporation.
//
// SPDX-License-Identifier: Apache-2.0

#include "ir/ir.h"

#include <gtest/gtest.h>

#include <sstream>

#include "frontends/p4/methodInstance.h"
#include "frontends/parsers/parserDriver.h"
#include "helpers.h"
#include "ir/pattern.h"

namespace P4::Test {

struct IrTest : P4CTest {};
struct ConstantTest : IrTest {};

#ifndef HAVE_LIBGC
TEST_F(IrTest, MethodResolutionReleasesItsExpression) {
    struct Receiver : IR::PathExpression {
        bool &destroyed;
        explicit Receiver(bool &destroyed)
            : IR::PathExpression(new IR::Type_Header("H", IR::IndexedVector<IR::StructField>{}),
                                 "header"),
              destroyed(destroyed) {}
        ~Receiver() override { destroyed = true; }
    };
    bool destroyed = false;
    auto *member = new IR::Member(new Receiver(destroyed), IR::Type_Header::isValid);
    member->type =
        new IR::Type_Method(IR::Type_Boolean::get(), new IR::ParameterList(), "isValid"_cs);
    auto *call = new IR::MethodCallExpression(member, new IR::Vector<IR::Argument>());
    auto instance = MethodInstance::resolve(call, nullptr);
    ASSERT_TRUE(instance->is<BuiltInMethod>());
    EXPECT_FALSE(destroyed);
    instance.reset();
    EXPECT_TRUE(destroyed);
}
#endif

TEST_F(IrTest, ComposedPatternsOutliveIntermediatePatterns) {
    Pattern::Match<IR::Constant> matched;
    auto pattern = [&] {
        auto sum = matched + 1;
        return sum * Pattern(2);
    }();
    IR::Ptr<IR::Expression> expression =
        new IR::Mul(new IR::Add(new IR::Constant(7), new IR::Constant(1)), new IR::Constant(2));
    ASSERT_TRUE(pattern.match(expression));
    EXPECT_EQ(matched->value, 7);
}

TEST_F(IrTest, ParsedProgramSourcesSurviveRepeatedVisits) {
    std::istringstream source("header H { bit<8> value; }");
    auto [program, sources] = P4ParserDriver::parseProgramSources(source, "ownership.p4");
    ASSERT_NE(program, nullptr);
    ASSERT_NE(sources, nullptr);
    Inspector visitor;
    program->apply(visitor);
    program->apply(visitor);
    ASSERT_EQ(program->objects.size(), 1U);
    EXPECT_EQ(program->objects.front()->checkedTo<IR::Type_Header>()->name.name, "H");
}

TEST_F(IrTest, InternedLiteralsOutliveTheirConsumers) {
    auto bits = IR::Type_Bits::get(8);
    const auto *constant = IR::Constant::get(bits, 42);
    const auto *string = IR::StringLiteral::get("retained-literal"_cs);
    {
        [[maybe_unused]] IR::Ptr<IR::Expression> constantOwner = constant;
        [[maybe_unused]] IR::Ptr<IR::Expression> stringOwner = string;
    }
    EXPECT_EQ(IR::Constant::get(bits, 42), constant);
    EXPECT_EQ(IR::StringLiteral::get("retained-literal"_cs), string);
    EXPECT_EQ(constant->value, 42);
    EXPECT_EQ(string->value, "retained-literal"_cs);
}

TEST_F(ConstantTest, ConstantInt0) {
    RedirectStderr errors;
    auto con = IR::Constant::get(IR::Type_Bits::get(0, true), 1);
    errors.dumpAndReset();
    EXPECT_TRUE(errors.contains("warn=overflow"));
    EXPECT_TRUE(errors.contains("cannot have non-zero value"));
    EXPECT_EQ(P4::warningCount(), 1);
    EXPECT_EQ(con->value, 0);
}

TEST_F(ConstantTest, ConstantInt1Overflow) {
    RedirectStderr errors;
    auto con = IR::Constant::get(IR::Type_Bits::get(1, true), 1);
    errors.dumpAndReset();
    EXPECT_TRUE(errors.contains("warn=overflow"));
    EXPECT_TRUE(errors.contains("signed value does not fit"));
    EXPECT_EQ(P4::warningCount(), 1);
    EXPECT_EQ(con->value, -1);
}

TEST_F(ConstantTest, ConstantInt1NoOverflow) {
    auto con = IR::Constant::get(IR::Type_Bits::get(1, true), 0);
    EXPECT_EQ(P4::warningCount(), 0);
    EXPECT_EQ(con->value, 0);
}

TEST_F(ConstantTest, ConstantBit1Overflow) {
    RedirectStderr errors;
    auto con = IR::Constant::get(IR::Type_Bits::get(1, false), 2);
    errors.dumpAndReset();
    EXPECT_TRUE(errors.contains("warn=overflow"));
    EXPECT_TRUE(errors.contains("value does not fit"));
    EXPECT_EQ(P4::warningCount(), 1);
    EXPECT_EQ(con->value, 0);
}

TEST_F(ConstantTest, ConstantBit1OverflowNeg) {
    RedirectStderr errors;
    auto con = IR::Constant::get(IR::Type_Bits::get(1, false), -1);
    errors.dumpAndReset();
    EXPECT_TRUE(errors.contains("warn=mismatch"));
    EXPECT_TRUE(errors.contains("negative value with unsigned type"));
    EXPECT_EQ(P4::warningCount(), 1);
    EXPECT_EQ(con->value, 1);
}

TEST_F(ConstantTest, ConstantBit1NoOverflow) {
    auto con = IR::Constant::get(IR::Type_Bits::get(1, false), 1);
    EXPECT_EQ(P4::warningCount(), 0);
    EXPECT_EQ(con->value, 1);
}

}  // namespace P4::Test
