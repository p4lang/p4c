#include "backends/p4tools/modules/testgen/targets/bmv2/p4_asserts_parser.h"

#include <unistd.h>

#include <gtest/gtest.h>

#include <cstdlib>
#include <string>

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/common/compiler/midend.h"
#include "backends/p4tools/common/lib/variables.h"
#include "frontends/common/options.h"
#include "frontends/common/parseInput.h"
#include "frontends/common/parser_options.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/frontend.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "ir/irutils.h"
#include "lib/compile_context.h"
#include "lib/null.h"

#include "backends/p4tools/modules/testgen/targets/bmv2/p4_refers_to_parser.h"
#include "backends/p4tools/modules/testgen/test/gtest_utils.h"

/// Variables are declared in "test/gtest/env.h" which is already included in reachability.cpp
extern const char *sourcePath;
extern const char *buildPath;

namespace Test {
class P4AssertsParserTest : public P4ToolsTest {};
class P4TestOptions : public CompilerOptions {
 public:
    virtual ~P4TestOptions() = default;
    P4TestOptions() = default;
    P4TestOptions(const P4TestOptions &) = default;
    P4TestOptions(P4TestOptions &&) = delete;
    P4TestOptions &operator=(const P4TestOptions &) = default;
    P4TestOptions &operator=(P4TestOptions &&) = delete;
};
/// Vector containing pairs of restrictions and nodes to which these restrictions apply.
using P4TestContext = P4CContextWithOptions<P4TestOptions>;
using P4Tools::ConstraintsVector;

ConstraintsVector loadExample(const char *curFile, bool flag) {
    AutoCompileContext autoP4TestContext(new P4TestContext);
    auto &options = P4TestContext::get().options();
    const char *argv = "./gtest-p4testgen";
    options.process(1, (char *const *)&argv);
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    std::string includeDir = std::string(buildPath) + std::string("p4include");
    auto *originalEnv = getenv("P4C_16_INCLUDE_PATH");
    setenv("P4C_16_INCLUDE_PATH", includeDir.c_str(), 1);
    const IR::P4Program *program = nullptr;
    options.file = sourcePath;
    options.file += curFile;
    if (access(options.file, 0) != 0) {
        // Subpath for bf-p4c-compilers.
        options.file = sourcePath;
        options.file += curFile;
    }
    program = P4::parseP4File(options);
    if (originalEnv == nullptr) {
        unsetenv("P4C_16_INCLUDE_PATH");
    } else {
        setenv("P4C_16_INCLUDE_PATH", originalEnv, 1);
    }
    P4::FrontEnd frontend;
    program = frontend.run(options, program);
    CHECK_NULL(program);
    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;
    P4Tools::MidEnd midEnd(options);
    program = program->apply(midEnd);
    if (flag) {
        P4Tools::ConstraintsVector result;
        program->apply(P4Tools::P4Testgen::Bmv2::AssertsParser(result));
        return result;
    }
    P4Tools::P4Testgen::Bmv2::RefersToParser referstoParser;
    program->apply(referstoParser);
    return referstoParser.getRestrictionsVector();
}

TEST_F(P4AssertsParserTest, RestrictionCount) {
    ConstraintsVector parsingResult = loadExample(
        "backends/p4tools/modules/testgen/targets/bmv2/test/p4-programs/bmv2_restrictions_1.p4",
        true);
    ASSERT_EQ(parsingResult.size(), (unsigned long)3);
}

TEST_F(P4AssertsParserTest, Restrictions) {
    ConstraintsVector parsingResult = loadExample(
        "backends/p4tools/modules/testgen/targets/bmv2/test/p4-programs/bmv2_restrictions_1.p4",
        true);
    ASSERT_EQ(parsingResult.size(), (unsigned long)3);
    {
        const auto &expr1 = P4Tools::ToolsVariables::getSymbolicVariable(
            IR::Type_Bits::get(8), "ingress.ternary_table_mask_h.h.a1");
        const auto &expr2 = P4Tools::ToolsVariables::getSymbolicVariable(
            IR::Type_Bits::get(8), "ingress.ternary_table_lpm_prefix_h.h.a1");
        const auto *const1 = IR::getConstant(IR::Type_Bits::get(8), 0);
        const auto *const2 = IR::getConstant(IR::Type_Bits::get(8), 64);
        const auto *operation =
            new IR::LAnd(new IR::Neq(expr1, const1), new IR::Neq(expr2, const2));
        ASSERT_TRUE(parsingResult[0]->equiv(*operation));
    }
    {
        const auto &expr1 = P4Tools::ToolsVariables::getSymbolicVariable(
            IR::Type_Bits::get(8), "ingress.ternary_table_key_h.h.a1");
        const auto *const1 = IR::getConstant(IR::Type_Bits::get(8), 0);
        const auto *operation1 = new IR::Neq(expr1, const1);
        ASSERT_TRUE(parsingResult[1]->equiv(*operation1));
    }
    {
        const auto &expr1 = P4Tools::ToolsVariables::getSymbolicVariable(
            IR::Type_Bits::get(8), "ingress.ternary_table_key_h.h.a");
        const auto *const2 = IR::getConstant(IR::Type_Bits::get(8), 255);
        const auto *operation2 = new IR::Neq(expr1, const2);
        ASSERT_TRUE(parsingResult[2]->equiv(*operation2));
    }
}

TEST_F(P4AssertsParserTest, RestrictionMiddleblockReferToInTable) {
    ConstraintsVector parsingResult = loadExample(
        "backends/p4tools/modules/testgen/targets/bmv2/test/p4-programs/bmv2_restrictions_2.p4",
        false);
    ASSERT_EQ(parsingResult.size(), (unsigned long)3);
    const auto &expr1 = P4Tools::ToolsVariables::getSymbolicVariable(IR::Type_Bits::get(8),
                                                                     "ingress.table_1_key_h.h.a");
    const auto &expr2 = P4Tools::ToolsVariables::getSymbolicVariable(IR::Type_Bits::get(8),
                                                                     "ingress.table_2_key_h.h.b");
    const auto *operation = new IR::Equ(expr1, expr2);
    ASSERT_TRUE(parsingResult[0]->equiv(*operation));
}

TEST_F(P4AssertsParserTest, RestrictionMiddleblockReferToInAction) {
    ConstraintsVector parsingResult = loadExample(
        "backends/p4tools/modules/testgen/targets/bmv2/test/p4-programs/bmv2_restrictions_2.p4",
        false);
    ASSERT_EQ(parsingResult.size(), (unsigned long)3);
    const auto *expr1 = P4Tools::ToolsVariables::getSymbolicVariable(
        IR::Type_Bits::get(8), "ingress.table_1_ingress.MyAction1_arg_input_val");
    const auto *expr2 = P4Tools::ToolsVariables::getSymbolicVariable(IR::Type_Bits::get(8),
                                                                     "ingress.table_1_key_h.h.a");
    auto *operation = new IR::Equ(expr1, expr2);
    ASSERT_TRUE(parsingResult[1]->equiv(*operation));
}

}  // namespace Test
