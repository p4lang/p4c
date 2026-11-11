// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
//
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "frontends/common/parseInput.h"
#include "helpers.h"
#include "midend/copyStructures.h"
#include "midend/flattenUnions.h"
#include "midend/interpreter.h"

namespace P4::Test {
namespace {

// Execute the control after lowering. The existing expression evaluator handles values and
// header validity; this helper only selects branches and sequences statements.
void executeUnionTest(const IR::StatOrDecl *node, ReferenceMap &refMap, TypeMap &typeMap,
                      ValueMap &values) {
    if (const auto *block = node->to<IR::BlockStatement>()) {
        for (const auto *component : block->components)
            executeUnionTest(component, refMap, typeMap, values);
    } else if (const auto *assignment = node->to<IR::AssignmentStatement>()) {
        ExpressionEvaluator evaluator(&refMap, &typeMap, &values);
        auto *right = evaluator.evaluate(assignment->right, false)->clone();
        SymbolicValue *left;
        const auto *member = assignment->left->to<IR::Member>();
        if (member && typeMap.getType(member->expr, true)->is<IR::Type_Header>()) {
            // The evaluator rejects reads from invalid headers. A field write is legal and
            // must not change validity, so obtain the destination without reading the field.
            auto *header = evaluator.evaluate(member->expr, true)->to<SymbolicHeader>();
            ASSERT_NE(header, nullptr);
            left = header->SymbolicStruct::get(member, member->member.name);
        } else {
            left = evaluator.evaluate(assignment->left, true);
        }
        ASSERT_FALSE(right->is<SymbolicError>());
        ASSERT_FALSE(left->is<SymbolicError>());
        left->assign(right);
    } else if (const auto *branch = node->to<IR::IfStatement>()) {
        ExpressionEvaluator evaluator(&refMap, &typeMap, &values);
        auto *condition = evaluator.evaluate(branch->condition, false)->to<SymbolicBool>();
        ASSERT_NE(condition, nullptr);
        ASSERT_TRUE(condition->isKnown());
        const auto *selected = condition->value ? branch->ifTrue : branch->ifFalse;
        if (selected) executeUnionTest(selected, refMap, typeMap, values);
    } else if (const auto *call = node->to<IR::MethodCallStatement>()) {
        ExpressionEvaluator evaluator(&refMap, &typeMap, &values);
        ASSERT_FALSE(evaluator.evaluate(call->methodCall, false)->is<SymbolicError>());
    } else if (const auto *variable = node->to<IR::Declaration_Variable>()) {
        SymbolicValueFactory factory(&typeMap);
        values.set(variable, factory.create(typeMap.getType(variable, true), true));
        ASSERT_EQ(variable->initializer, nullptr);
    } else {
        FAIL() << "Unexpected statement: " << node;
    }
}

class HeaderUnionMidend : public P4CTest {};

TEST_F(HeaderUnionMidend, PreserveOperationsOnUnflattenedUnions) {
    auto *program = parseP4String(R"(
        header H { bit<8> value; }
        header_union U { H first; H second; }
        control C(inout U u, inout U[2] stack) {
            apply {
                u.first.setValid();
                u.second.setInvalid();
                u.second = u.first;
                u.second.value = 7;
                stack[0].first.setValid();
                stack[0].second.setInvalid();
                stack[0].second = stack[0].first;
            }
        }
    )");
    ASSERT_NE(program, nullptr);
    ReferenceMap refMap;
    TypeMap typeMap;
    program = program->apply(TypeInference(&typeMap, false));
    program = program->apply(TypeChecking(&refMap, &typeMap, true));
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(errorCount(), 0U);
    const auto *lowered = program->apply(HandleValidityHeaderUnion(&refMap, &typeMap, false));
    ASSERT_NE(lowered, nullptr);
    EXPECT_TRUE(program->equiv(*lowered));
}

TEST_F(HeaderUnionMidend, PreserveValidityAndValues) {
    for (bool expandUnions : {false, true}) {
        for (bool copyHeaders : {false, true}) {
            SCOPED_TRACE(expandUnions);
            SCOPED_TRACE(copyHeaders);
            auto &options = P4CContext::get().options();
            options.file = P4CTestEnvironment::getProjectRoot() /
                           "testdata/p4_16_samples/header-union-midend.p4";
            auto *program = parseP4File(options);
            ASSERT_NE(program, nullptr);
            ReferenceMap refMap;
            TypeMap typeMap;
            PassManager passes = {
                new TypeInference(&typeMap, false),
                new TypeChecking(&refMap, &typeMap, true),
                new CopyStructures(&typeMap,
                                   {.copyHeaders = copyHeaders, .expandUnions = expandUnions}),
                new FlattenHeaderUnion(&refMap, &typeMap),
                new TypeChecking(&refMap, &typeMap, true),
            };
            program = program->apply(passes);
            ASSERT_NE(program, nullptr);
            ASSERT_EQ(errorCount(), 0U);
            const auto *control =
                program->getDeclsByName("Exercise"_cs)->single()->checkedTo<IR::P4Control>();
            const auto *source = control->getApplyParameters()->getParameter("source"_cs);
            const auto *ok = control->getApplyParameters()->getParameter("ok"_cs);
            for (int active = 0; active < 3; ++active) {
                SCOPED_TRACE(active);
                ValueMap values;
                values.set(source,
                           new SymbolicInteger(new IR::Constant(IR::Type_Bits::get(2), active)));
                values.set(ok, new SymbolicBool());
                for (const auto *local : control->controlLocals) {
                    executeUnionTest(local->checkedTo<IR::StatOrDecl>(), refMap, typeMap, values);
                }
                executeUnionTest(control->body, refMap, typeMap, values);
                const auto *result = values.get(ok)->to<SymbolicBool>();
                ASSERT_NE(result, nullptr);
                ASSERT_TRUE(result->isKnown());
                EXPECT_TRUE(result->value);
            }
        }
    }
}

}  // namespace
}  // namespace P4::Test
