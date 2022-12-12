#include "backends/p4tools/common/compiler/p4_asserts_parser.h"

#include <stdlib.h>
#include <unistd.h>

#include <ext/alloc_traits.h>

#include <memory>
#include <string>
#include <vector>

#include "backends/p4test/version.h"
#include "backends/p4tools/common/compiler/midend.h"
#include "backends/p4tools/common/lib/util.h"
#include "frontends/common/options.h"
#include "frontends/common/parseInput.h"
#include "frontends/common/parser_options.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/frontend.h"
#include "frontends/p4/typeMap.h"
#include "gtest/gtest-message.h"
#include "gtest/gtest-test-part.h"
#include "gtest/gtest.h"
#include "ir/ir.h"
#include "ir/irutils.h"
#include "lib/compile_context.h"
#include "lib/null.h"
#include "p4tools/common/lib/formulae.h"

#include "backends/p4tools/testgen/targets/bmv2/p4_refers_to_parser.h"
#include "backends/p4tools/testgen/test/gtest_utils.h"

/// Variables are declared in "test/gtest/env.h" which is already included in reachablity.cpp
extern const char* sourcePath;
extern const char* buildPath;

namespace Test {
class P4ExpressionParserTest : public P4ToolsTest {};
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

const IR::P4Program* loadExample(const char* curFile) {
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
    return program->apply(midEnd);
}

TEST_F(P4ExpressionParserTest, SimpleExpressions) {
    const auto* program = loadExample(
        "backends/p4tools/testgen/targets/bmv2/test/p4-programs/bmv2_if.p4");
    // Check local variable
    const auto* expr = ExpressionParser::Parser::getIR("ingress::d", program)->to<IR::Expression>();
    ASSERT_BOOL(expr->is<IR::PathExpression>());
    ASSERT_BOOL(expr->to<IR::PathExpression>()->path->name.originalName == "d");
    ASSERT_BOOL(expr->type->is<IR::Type_Bits>());
    const auto* type = expr->type->to<IR::Type_Bits>();;
    ASSERT_EQ(type->size, 8);

    // Check member.
    expr = ExpressionParser::Parser::getIR("egress::h.h.b", program)->to<IR::Expression>();
    ASSERT_BOOL(expr->is<IR::Member>());
    const auto* member = expr->to<IR::Member>();
    ASSERT_BOOL(member->member->name.originalName == "b");
    ASSERT_BOOL(member->type->is<IR::Type_Bits>());
    type = member->type->to<IR::Type_Bits>();
    ASSERT_EQ(type->size, 8);
    ASSERT_BOOL(member->expr->is<IR::Member>());
    member = member->expr->to<IR::Member>();
    ASSERT_BOOL(member->member->originalName == "h");
    ASSERT_BOOL(member->type->is<IR::Type_StructLike>());
    const auto* strct = member->type->to<IR::Type_StructLike>();
    ASSERT_BOOL(strct->originalName == "H");
    ASSERT_BOOL(member->expr->is<IR::PathExpression>());
    const auto* pathExpression = member->expr->to<IR::PathExpression>();
    ASSERT_BOOL(pathExpression->path->name.originalName == "h");
    ASSERT_BOOL(pathExpression->type->is<IR::Type_StructLike>());
    const auto* strct = pathExpression->type->to<IR::Type_StructLike>();
    ASSERT_BOOL(strct->originalName == "Headers");

    // Check true/false.
    expr = ExpressionParser::Parser::getIR("true", program)->to<IR::Expression>();
    ASSERT_BOOL(expr->is<IR::BoolLiteral>());
    const auto* boolLiteral = expr->to<IR::BoolLiteral>();
    ASSERT_BOOL(boolLiteral->value);
    expr = ExpressionParser::Parser::getIR("false", program)->to<IR::Expression>();
    ASSERT_BOOL(expr->is<IR::BoolLiteral>());
    const auto* boolLiteral = expr->to<IR::BoolLiteral>();
    ASSERT_BOOL(!boolLiteral->value);

    // Check simple constant.
    expr = ExpressionParser::Parser::getIR("10", program)->to<IR::Expression>();
    ASSERT_BOOL(expr->is<IR::Constant>())
    const auto* cnsnt = expr->to<IR::Constant>();
    ASSERT_BOOL(!cnsnt->value == 10);

    // Check hex constant.
    expr = ExpressionParser::Parser::getIR("0xaB", program)->to<IR::Expression>();
    ASSERT_BOOL(expr->is<IR::Constant>())
    const auto* cnsnt = expr->to<IR::Constant>();
    ASSERT_BOOL(cnsnt->value == 171);

    // check method call
    // check slice
    // check array
    // logical not and or, and implication
    // complement
    // mul, div, mod, %
    /// -, |-|, +, |+|
    // <, <=, >, >=
    // ==, !=
    // <<, >>
    // (x + y) * 2
    // x - y - z
}

}  // Test