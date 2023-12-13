#include "frontends/common/constantFolding.h"
#include "frontends/common/parseInput.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/validateSerEnums.h"
#include "gtest/gtest.h"
#include "helpers.h"
#include "ir/ir.h"
#include "ir/pass_manager.h"
#include "lib/log.h"

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

struct P4CFrontendEnumValidation : P4CFrontend {
    P4CFrontendEnumValidation() {
        addPasses({new P4::ResolveReferences(&refMap), new P4::ConstantFolding(&refMap, nullptr),
                   new P4::ValidateSerEnums(&refMap)});
    }

    P4::ReferenceMap refMap;
};

TEST_F(P4CFrontendEnumValidation, Bit) {
    std::string program = P4_SOURCE(R"(
        enum bit<8> FailingExample {
            zero = 0,
            representable = 255,
            unrepresentable = 256
        }
    )");
    RedirectStderr errors;
    const auto *prog = parseAndProcess(program);
    ASSERT_TRUE(prog) << errors.str();
    ASSERT_EQ(::errorCount(), 1) << errors.str();
    ASSERT_TRUE(errors.contains("unrepresentable = 256"));
}

TEST_F(P4CFrontendEnumValidation, IntPos) {
    std::string program = P4_SOURCE(R"(
        enum int<8> FailingExample {
            representable_p = 127,
            representable_n = -128,
            unrepresentable_p = 128,
        }
    )");
    RedirectStderr errors;
    const auto *prog = parseAndProcess(program);
    ASSERT_TRUE(prog) << errors.str();
    ASSERT_EQ(::errorCount(), 1) << errors.str();
    ASSERT_TRUE(errors.contains("unrepresentable_p = 128"));
}

TEST_F(P4CFrontendEnumValidation, IntNeg) {
    std::string program = P4_SOURCE(R"(
        enum int<8> FailingExample {
            representable_p = 127,
            representable_n = -128,
            unrepresentable_n = -129
        }
    )");
    RedirectStderr errors;
    const auto *prog = parseAndProcess(program);
    ASSERT_TRUE(prog) << errors.str();
    ASSERT_EQ(::errorCount(), 1) << errors.str();
    ASSERT_TRUE(errors.contains("unrepresentable_n = -129"));
}

TEST_F(P4CFrontendEnumValidation, TypeDef) {
    std::string program = P4_SOURCE(R"(
        typedef int<7> TD1;
        typedef TD1 TD2;
        typedef TD2 TD3;
        enum TD3 FailingExample {
            representable_p = 63,
            representable_n = -64,
            unrepresentable_p = 64,
            unrepresentable_n = -65,
        }
    )");
    RedirectStderr errors;
    const auto *prog = parseAndProcess(program);
    ASSERT_TRUE(prog) << errors.str();
    ASSERT_EQ(::errorCount(), 2) << errors.str();
    ASSERT_TRUE(errors.contains("unrepresentable_p = 64"));
    ASSERT_TRUE(errors.contains("unrepresentable_n = -65"));
}

TEST_F(P4CFrontendEnumValidation, ExplicitCast) {
    std::string program = P4_SOURCE(R"(
        enum int<8> FailingExample {
            explicit_cast = (int<8>)-1000
        }
    )");
    RedirectStderr errors;
    const auto *prog = parseAndProcess(program);
    ASSERT_TRUE(prog) << errors.str();
    ASSERT_EQ(::errorCount(), 0) << errors.str();
}

}  // namespace Test
