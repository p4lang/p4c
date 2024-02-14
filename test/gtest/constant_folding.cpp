#include <gtest/gtest.h>

#include "frontends/common/constantFolding.h"
#include "frontends/common/parseInput.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "helpers.h"
#include "ir/ir.h"
#include "ir/pass_manager.h"
#include "lib/log.h"

using namespace P4;

namespace Test {

struct P4CFrontend : P4CTest {
    void addPasses(std::initializer_list<PassManager::VisitorRef> passes) { pm.addPasses(passes); }

    const IR::Node *parseAndProcess(std::string program) {
        const auto *pgm = P4::parseP4String(program, CompilerOptions::FrontendVersion::P4_16);
        EXPECT_TRUE(pgm);
        EXPECT_EQ(::errorCount(), 0);
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
            new P4::ResolveReferences(&refMap),
            new P4::ConstantFolding(&refMap, nullptr, policy),
        });
    }

    P4::ReferenceMap refMap;
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
    for (const auto *d : *program->getDeclarations()) {
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
    auto *sf_1 = getStructField(program, "my_hdr_stack_1_t", "hdr");
    ASSERT_TRUE(sf_1);
    auto *ts_1 = sf_1->type->to<IR::Type_Stack>();
    ASSERT_TRUE(ts_1->size->is<IR::Constant>());

    // my_hdr_stack_2_t.hdr should be a Constant
    auto *sf_2 = getStructField(program, "my_hdr_stack_2_t", "hdr");
    ASSERT_TRUE(sf_2);
    auto *ts_2 = sf_2->type->to<IR::Type_Stack>();
    ASSERT_TRUE(ts_2->size->is<IR::Constant>());
}

// Constant folding with filtering active
TEST_F(P4CConstantFoldingValidation, filter) {
    // Create the passes with a filter to skip processing of some structs
    createPasses(new ConstantFoldFilter());

    auto *program = parseAndProcess(constantFoldFilterCode())->to<IR::P4Program>();
    ASSERT_TRUE(program);

    // my_hdr_stack_1_t.hdr should be a Constant
    auto *sf_1 = getStructField(program, "my_hdr_stack_1_t", "hdr");
    ASSERT_TRUE(sf_1);
    auto *ts_1 = sf_1->type->to<IR::Type_Stack>();
    EXPECT_TRUE(ts_1->size->is<IR::PathExpression>());

    // my_hdr_stack_2_t.hdr should be a Constant
    auto *sf_2 = getStructField(program, "my_hdr_stack_2_t", "hdr");
    ASSERT_TRUE(sf_2);
    auto *ts_2 = sf_2->type->to<IR::Type_Stack>();
    EXPECT_TRUE(ts_2->size->is<IR::Constant>());
}

}  // namespace Test
