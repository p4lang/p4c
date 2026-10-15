#include <gtest/gtest.h>

#include <cstdio>
#include <memory>

#include "frontends/common/constantFolding.h"
#include "frontends/common/parseInput.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/moveDeclarations.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/parsers/parserDriver.h"
#include "helpers.h"
#include "ir/ir.h"
#include "ir/pass_manager.h"
#include "lib/log.h"

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

struct P4CFrontendEnumValidation : P4CFrontend {
    P4CFrontendEnumValidation() {
        addPasses({new P4::DoConstantFolding(), new P4::TypeInference(&typeMap, false, false)});
    }

    P4::TypeMap typeMap;
};

TEST_F(P4CFrontend, ParseStdioAcrossBufferBoundaries) {
    std::unique_ptr<FILE, int (*)(FILE *)> input(std::tmpfile(), &std::fclose);
    ASSERT_NE(input, nullptr);
    // Start after a stdio read to exercise buffering on the caller's FILE*.
    const std::string source = "x/*" + std::string(16384, ' ') + "*/\nconst bit<8> value = 42;";
    ASSERT_EQ(std::fwrite(source.data(), 1, source.size(), input.get()), source.size());
    std::rewind(input.get());
    ASSERT_EQ(std::fgetc(input.get()), 'x');

    const auto *program = P4ParserDriver::parse(input.get(), "stdio.p4");
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->objects.size(), 1U);
    const auto *constant = program->objects.at(0)->to<IR::Declaration_Constant>();
    ASSERT_NE(constant, nullptr);
    EXPECT_EQ(constant->name, "value");
    EXPECT_EQ(constant->initializer->checkedTo<IR::Constant>()->value, 42);
    EXPECT_EQ(errorCount(), 0U);

    // Parsing must not close the caller's FILE*.
    std::rewind(input.get());
    EXPECT_EQ(std::fgetc(input.get()), 'x');
}

TEST_F(P4CFrontend, ParseEmptyStdio) {
    std::unique_ptr<FILE, int (*)(FILE *)> input(std::tmpfile(), &std::fclose);
    ASSERT_NE(input, nullptr);
    const auto *program = P4ParserDriver::parse(input.get(), "empty.p4");
    ASSERT_NE(program, nullptr);
    EXPECT_TRUE(program->objects.empty());
    EXPECT_EQ(errorCount(), 0U);
}

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
    errors.dumpAndReset();
    ASSERT_TRUE(prog);
    ASSERT_EQ(::P4::errorCount(), 1);
    ASSERT_TRUE(errors.contains("unrepresentable = 256"));
}

TEST_F(P4CFrontendEnumValidation, BitNeg) {
    std::string program = P4_SOURCE(R"(
        enum bit<8> FailingExample {
            zero = 0,
            representable = 255,
            unrepresentable = -1
        }
    )");
    RedirectStderr errors;
    const auto *prog = parseAndProcess(program);
    errors.dumpAndReset();
    ASSERT_TRUE(prog);
    ASSERT_EQ(::P4::errorCount(), 1);
    ASSERT_TRUE(errors.contains("unrepresentable = -1"));
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
    errors.dumpAndReset();
    ASSERT_TRUE(prog);
    ASSERT_EQ(::P4::errorCount(), 1);
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
    errors.dumpAndReset();
    ASSERT_TRUE(prog);
    ASSERT_EQ(::P4::errorCount(), 1);
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
    errors.dumpAndReset();
    ASSERT_TRUE(prog);
    ASSERT_EQ(::P4::errorCount(), 2);
    ASSERT_TRUE(errors.contains("unrepresentable_p = 64"));
    ASSERT_TRUE(errors.contains("unrepresentable_n = -65"));
    std::clog << errors.str();
}

TEST_F(P4CFrontendEnumValidation, ExplicitCast) {
    std::string program = P4_SOURCE(R"(
        enum int<8> FailingExample {
            explicit_cast = (int<8>)-1000
        }
    )");
    RedirectStderr errors;
    const auto *prog = parseAndProcess(program);
    errors.dumpAndReset();
    ASSERT_TRUE(prog);
    ASSERT_EQ(::P4::errorCount(), 0);
}

TEST_F(P4CFrontendEnumValidation, InvalidUnderlyingUnsized) {
    std::string program = P4_SOURCE(R"(
        enum int FailingExample {
            val = 4
        }
    )");
    RedirectStderr errors;
    const auto *prog = parseAndProcess(program);
    errors.dumpAndReset();
    ASSERT_TRUE(prog);
    ASSERT_EQ(::P4::errorCount(), 1);
    ASSERT_TRUE(errors.contains("Illegal type for enum;"));
    ASSERT_TRUE(errors.contains("is unsized integral"));
}

TEST_F(P4CFrontendEnumValidation, InvalidType) {
    std::string program = P4_SOURCE(R"(
        type bit<4> MyBit4;
        enum MyBit4 FailingExample {
            val = 4
        }
    )");
    RedirectStderr errors;
    const auto *prog = parseAndProcess(program);
    errors.dumpAndReset();
    ASSERT_TRUE(prog);
    ASSERT_EQ(::P4::errorCount(), 1);
    ASSERT_TRUE(errors.contains("Illegal type for enum;"));
    ASSERT_TRUE(errors.contains("type-declared types"));
}

// Tests for MoveInitializers
struct P4CFrontendMoveInitializers : P4CFrontend {
    P4CFrontendMoveInitializers() { addPasses({new P4::MoveInitializers()}); }
};

TEST_F(P4CFrontendMoveInitializers, P4ControlSrcInfo) {
    std::string program = P4_SOURCE(R"(
        control m() {
            bool blah = false;
            apply{}
        }
    )");
    const auto *prog = parseAndProcess(program);
    ASSERT_TRUE(prog);
    ASSERT_EQ(::P4::errorCount(), 0);

    // The P4Control->body should have a valid srcInfo if the information
    // is correctly maintained by MoveInitializers.
    const auto *p4prog = prog->to<IR::P4Program>();
    ASSERT_TRUE(p4prog);
    for (const auto *node : p4prog->objects) {
        if (const auto *control = node->to<IR::P4Control>()) {
            ASSERT_TRUE(control->body->srcInfo.isValid());
        }
    }
}

}  // namespace P4::Test
