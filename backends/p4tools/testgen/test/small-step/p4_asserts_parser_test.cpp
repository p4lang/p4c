#include "backends/p4tools/testgen/targets/bmv2/p4_asserts_parser.h"

#include <stdlib.h>

#include <fstream>
#include <memory>
#include <utility>
#include <vector>

#include <boost/optional/optional.hpp>

#include "backends/p4test/version.h"
#include "backends/p4tools/common/compiler/midend.h"
#include "backends/p4tools/common/lib/ir.h"
#include "frontends/common/parseInput.h"
#include "frontends/p4/frontend.h"
#include "gtest/gtest-message.h"
#include "gtest/gtest-test-part.h"
#include "gtest/gtest.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "lib/log.h"
#include "test/gtest/env.h"

#include "backends/p4tools/testgen/targets/bmv2/p4_refers_to_parser.h"
#include "backends/p4tools/testgen/test/gtest_utils.h"
#include "backends/p4tools/testgen/test/small-step/util.h"

namespace Test {
class P4AssertsParserTest : public P4ToolsTest {};
class P4TestOptions : public CompilerOptions {
 public:
    virtual ~P4TestOptions() = default;
    P4TestOptions() = default;
    P4TestOptions(const P4TestOptions&) = default;
    P4TestOptions(P4TestOptions&&) = delete;
    P4TestOptions& operator=(const P4TestOptions&) = default;
    P4TestOptions& operator=(P4TestOptions&&) = delete;
};
/// Vector containing pairs of restrictions and nodes to which these restrictions apply.
using P4TestContext = P4CContextWithOptions<P4TestOptions>;
using Restrictions = std::vector<std::vector<const IR::Expression*>>;

Restrictions loadExample(const char* curFile, bool flag) {
    AutoCompileContext autoP4TestContext(new P4TestContext);
    auto& options = P4TestContext::get().options();
    const char* argv = "./gtest-p4testgen";
    options.process(1, (char* const*)&argv);
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    std::string includeDir = std::string(buildPath) + std::string("p4include");
    auto* originalEnv = getenv("P4C_16_INCLUDE_PATH");
    setenv("P4C_16_INCLUDE_PATH", includeDir.c_str(), 1);
    options.compilerVersion = P4TEST_VERSION_STRING;
    const IR::P4Program* program = nullptr;
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
    Restrictions result;
    if (flag)
        program->apply(P4Tools::AssertsParser::AssertsParser(result));
    else
        program->apply(P4Tools::RefersToParser::RefersToParser(result));
    return result;
}

TEST_F(P4AssertsParserTest, RestrictionCount) {
    Restrictions parsingResult = loadExample(
        "backends/p4tools/testgen/targets/bmv2/test/p4-programs/bmv2_restrictions.p4", true);
    ASSERT_EQ(parsingResult.size(), (unsigned long)1);
    ASSERT_EQ(parsingResult[0].size(), (unsigned long)3);
}

TEST_F(P4AssertsParserTest, Restrictions) {
    Restrictions parsingResult = loadExample(
        "backends/p4tools/testgen/targets/bmv2/test/p4-programs/bmv2_restrictions.p4", true);
    ASSERT_EQ(parsingResult.size(), (unsigned long)1);
    auto expr1 = P4Tools::IRUtils::getZombieConst(IR::Type_Bits::get(8), 0,
                                                  "ingress.ternary_table_mask_h.h.a1");
    auto expr2 = P4Tools::IRUtils::getZombieConst(IR::Type_Bits::get(8), 0,
                                                  "ingress.ternary_table_lpm_prefix_h.h.a1");
    auto const1 = P4Tools::IRUtils::getConstant(IR::Type_Bits::get(8), 0);
    auto const2 = P4Tools::IRUtils::getConstant(IR::Type_Bits::get(8), 64);
    auto operation = new IR::LAnd(new IR::Neq(expr1, const1), new IR::Neq(expr2, const2));
    expr1 = P4Tools::IRUtils::getZombieConst(IR::Type_Bits::get(8), 0,
                                             "ingress.ternary_table_key_h.h.a1");
    auto operation1 = new IR::Neq(expr1, const1);
    expr1 = P4Tools::IRUtils::getZombieConst(IR::Type_Bits::get(8), 0,
                                             "ingress.ternary_table_key_h.h.a");
    const2 = P4Tools::IRUtils::getConstant(IR::Type_Bits::get(8), 255);
    auto operation2 = new IR::Neq(expr1, const2);
    ASSERT_TRUE(parsingResult[0][0]->equiv(*operation));
    ASSERT_TRUE(parsingResult[0][1]->equiv(*operation1));
    ASSERT_TRUE(parsingResult[0][2]->equiv(*operation2));
}

TEST_F(P4AssertsParserTest, RestrictionMiddleblockReferToInTable) {
    Restrictions parsingResult = loadExample(
        "backends/p4tools/testgen/targets/bmv2/test/p4-programs/bmv2_restrictions_2.p4", false);
    ASSERT_EQ(parsingResult.size(), (unsigned long)2);
    auto expr1 =
        P4Tools::IRUtils::getZombieConst(IR::Type_Bits::get(8), 0, "ingress.table_1_key_h.h.a");
    auto expr2 =
        P4Tools::IRUtils::getZombieConst(IR::Type_Bits::get(8), 0, "ingress.table_2_key_h.h.a");
    auto operation = new IR::Equ(expr1, expr2);
    ASSERT_TRUE(parsingResult[0][0]->equiv(*operation));
}

TEST_F(P4AssertsParserTest, RestrictionMiddleblockReferToInAction) {
    Restrictions parsingResult = loadExample(
        "backends/p4tools/testgen/targets/bmv2/test/p4-programs/bmv2_restrictions_2.p4", false);
    ASSERT_EQ(parsingResult.size(), (unsigned long)2);
    auto expr1 = P4Tools::IRUtils::getZombieConst(IR::Type_Bits::get(8), 0,
                                                  "ingress.table_1_param_ingress.MyAction10");
    auto expr2 =
        P4Tools::IRUtils::getZombieConst(IR::Type_Bits::get(8), 0, "ingress.table_1_key_h.h.a");
    auto operation = new IR::Equ(expr1, expr2);
    ASSERT_TRUE(parsingResult[1][0]->equiv(*operation));
}

}  // namespace Test
