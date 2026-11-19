// SPDX-FileCopyrightText: 2024 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "frontends/common/constantFolding.h"
#include "frontends/common/parseInput.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "helpers.h"
#include "ir/ir.h"
#include "ir/pass_manager.h"
#include "lib/log.h"

using namespace P4;

namespace P4::Test {

struct P4CFrontend : P4CTest {
    void addPasses(std::initializer_list<PassManager::VisitorRef> passes) { pm.addPasses(passes); }

    const IR::Node *parseAndProcess(std::string program) {
        const auto *pgm = P4::parseP4String(program);
        EXPECT_TRUE(pgm);
        EXPECT_EQ(::P4::errorCount(), 0);
        if (!pgm) {
            return nullptr;
        }
        return pgm->apply(pm);
    }

    PassManager pm;
};

struct P4CConstantFoldingValidation : P4CFrontend {
    void createPasses(P4::ConstantFoldingPolicy *policy) {
        addPasses({
            new P4::ConstantFolding(policy),
        });
    }
};

/**
 * Filter to prevent processing of fields in the "my_hdr_stack_1_t" struct
 */
class ConstantFoldFilter : public P4::ConstantFoldingPolicy {
 public:
    const IR::Node *hook(Visitor &v, IR::PathExpression *pe) override {
        auto ts = v.findContext<IR::Type_Struct>();
        if (ts && ts->name == "my_hdr_stack_1_t") {
            return pe;
        }
        return nullptr;
    }
};

const IR::StructField *getStructField(const IR::P4Program *program, cstring struct_name,
                                      cstring field_name) {
    for (const auto *d : program->getDeclarations()) {
        const auto *ts = d->to<IR::Type_Struct>();
        if (ts && ts->name == struct_name) {
            for (const auto *f : ts->fields) {
                const auto *sf = f->to<IR::StructField>();
                if (sf && sf->name == field_name) {
                    return sf;
                }
            }
        }
    }
    return nullptr;
}

const std::string constantFoldFilterCode() {
    static const std::string code = P4_SOURCE(R"(
        const bit<16> CONST_1 = 3;
        const bit<16> CONST_2 = 4;

        header my_hdr_t {
            bit<16>   field;
        }

        struct my_hdr_stack_1_t {
            my_hdr_t[CONST_1]    hdr;
        }

        struct my_hdr_stack_2_t {
            my_hdr_t[CONST_2]    hdr;
        }
    )");

    return code;
}

// Constant folding without any filter
TEST_F(P4CConstantFoldingValidation, no_filter) {
    // Create the passes without any ConstantFoldingPolicy
    createPasses(nullptr);

    auto *program = parseAndProcess(constantFoldFilterCode())->to<IR::P4Program>();
    ASSERT_TRUE(program);

    // my_hdr_stack_1_t.hdr should be a Constant
    auto *sf_1 = getStructField(program, "my_hdr_stack_1_t"_cs, "hdr"_cs);
    ASSERT_TRUE(sf_1);
    auto *ts_1 = sf_1->type->to<IR::Type_Array>();
    ASSERT_TRUE(ts_1->size->is<IR::Constant>());

    // my_hdr_stack_2_t.hdr should be a Constant
    auto *sf_2 = getStructField(program, "my_hdr_stack_2_t"_cs, "hdr"_cs);
    ASSERT_TRUE(sf_2);
    auto *ts_2 = sf_2->type->to<IR::Type_Array>();
    ASSERT_TRUE(ts_2->size->is<IR::Constant>());
}

TEST_F(P4CConstantFoldingValidation, struct_cast_preserves_type) {
    createPasses(nullptr);

    std::string code = P4_SOURCE(R"(
        struct S {
            bit<8> x;
        }

        const S value = (S){x = 1};
    )");

    auto *program = parseAndProcess(code)->to<IR::P4Program>();
    ASSERT_TRUE(program);

    const IR::Declaration_Constant *constant = nullptr;
    for (const auto *declaration : program->getDeclarations()) {
        const auto *candidate = declaration->to<IR::Declaration_Constant>();
        if (candidate != nullptr && candidate->name == "value") {
            constant = candidate;
            break;
        }
    }
    ASSERT_TRUE(constant);

    const auto *initializer = constant->initializer->to<IR::StructExpression>();
    ASSERT_TRUE(initializer);
    ASSERT_TRUE(initializer->structType);
    EXPECT_TRUE(initializer->structType->is<IR::Type_Name>());
}

// Constant folding with filtering active
TEST_F(P4CConstantFoldingValidation, filter) {
    // Create the passes with a filter to skip processing of some structs
    createPasses(new ConstantFoldFilter());

    auto *program = parseAndProcess(constantFoldFilterCode())->to<IR::P4Program>();
    ASSERT_TRUE(program);

    // my_hdr_stack_1_t.hdr should be a Constant
    auto *sf_1 = getStructField(program, "my_hdr_stack_1_t"_cs, "hdr"_cs);
    ASSERT_TRUE(sf_1);
    auto *ts_1 = sf_1->type->to<IR::Type_Array>();
    EXPECT_TRUE(ts_1->size->is<IR::PathExpression>());

    // my_hdr_stack_2_t.hdr should be a Constant
    auto *sf_2 = getStructField(program, "my_hdr_stack_2_t"_cs, "hdr"_cs);
    ASSERT_TRUE(sf_2);
    auto *ts_2 = sf_2->type->to<IR::Type_Array>();
    EXPECT_TRUE(ts_2->size->is<IR::Constant>());
}

namespace {

/// Collects assigned values.
class AssignedValues : public Inspector {
 public:
    std::vector<const IR::Expression *> values;

 private:
    bool preorder(const IR::AssignmentStatement *assign) override {
        values.push_back(assign->right);
        return false;
    }
};

const char *const structDecls = R"(
    struct S { bit<8> a; bit<16> b; }
    struct N { S s; bool f; }
    const S s1 = { a = 1, b = 2 };
    const S s2 = { a = 1, b = 3 };
    const S s1r = { b = 2, a = 1 };
    const N n1 = { s = { a = 1, b = 2 }, f = true };
    const N n2 = { f = true, s = { b = 2, a = 1 } };
    const N n3 = { s = { a = 1, b = 3 }, f = true };
)";

/// Folds @p expr to a bool, or nullopt.
std::optional<bool> foldBool(const std::string &expr) {
    // Fresh context per program.
    AutoCompileContext autoContext(std::make_unique<GTestContext>());
    const std::string source = std::string(structDecls) + "const bool result = " + expr + ";\n" +
                               R"(
        control C(out bool o);
        package Top(C c);
        control c(out bool o) { apply { o = result; } }
        Top(c()) main;
    )";
    auto test = FrontendTestCase::create(P4_SOURCE(source.c_str()));
    if (!test) return std::nullopt;
    AssignedValues assigned;
    test->program->apply(assigned);
    if (assigned.values.size() != 1) return std::nullopt;
    const auto *value = assigned.values.front()->to<IR::BoolLiteral>();
    if (value == nullptr) return std::nullopt;
    return value->value;
}

}  // namespace

struct P4CConstantFoldingComparison : P4CTest {};

TEST_F(P4CConstantFoldingComparison, StructEquality) {
    const std::vector<std::pair<std::string, bool>> cases = {
        {"s1 == s1", true},
        {"s1 == s2", false},
        {"s1 != s2", true},
        {"s1 != s1", false},
        {"s1 == { a = 1, b = 2 }", true},
        {"s1 == { 1, 3 }", false},
        {"n1 == n3", false},
        {"n1 != n3", true},
        {"n1.s == s1", true},
        {"n3.s == s1", false},
    };
    for (const auto &[expr, expected] : cases) {
        EXPECT_EQ(std::optional<bool>(expected), foldBool(expr)) << expr;
    }
}

TEST_F(P4CConstantFoldingComparison, StructEqualityIgnoresInitializerFieldOrder) {
    EXPECT_EQ(std::optional<bool>(true), foldBool("s1 == s1r"));
    EXPECT_EQ(std::optional<bool>(false), foldBool("s1 != s1r"));
    EXPECT_EQ(std::optional<bool>(true), foldBool("n1 == n2"));
}

TEST_F(P4CConstantFoldingComparison, StructMemberComparedToLiteral) {
    const std::vector<std::pair<std::string, bool>> cases = {
        {"s1.a == 1", true},           {"1 == s1.a", true},
        {"s1.b != 3", true},           {"s1.a < 5", true},
        {"s1.a > 1", false},           {"s1.a >= 2", false},
        {"n1.s.b <= 2 && n1.f", true},
    };
    for (const auto &[expr, expected] : cases) {
        EXPECT_EQ(std::optional<bool>(expected), foldBool(expr)) << expr;
    }
}

TEST_F(P4CConstantFoldingComparison, NonConstantOperandIsNotCompileTimeConstant) {
    std::string source = P4_SOURCE(R"(
        struct S { bit<8> a; }
        control c() {
            S v = { a = 1 };
            const bool r = v.a == 1;
            apply { }
        }
    )");
    EXPECT_FALSE(FrontendTestCase::create(source));
}

}  // namespace P4::Test
