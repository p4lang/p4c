#include "backends/p4tools/common/compiler/p4_expr_parser.h"

#include <stdlib.h>
#include <unistd.h>

#include <ext/alloc_traits.h>

#include <memory>
#include <string>
#include <vector>

#include "backends/p4test/version.h"
#include "backends/p4tools/common/compiler/midend.h"
#include "backends/p4tools/common/lib/formulae.h"
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

#include "backends/p4tools/modules/testgen/targets/bmv2/p4_refers_to_parser.h"
#include "backends/p4tools/modules/testgen/test/gtest_utils.h"

/// Variables are declared in "test/gtest/env.h" which is already included in reachablity.cpp
extern const char *sourcePath;
extern const char *buildPath;

namespace Test {
class P4ExpressionParserTest : public P4ToolsTest {};
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
using Restrictions = std::vector<std::vector<const IR::Expression *>>;

const IR::P4Program *loadExample(const char *curFile) {
    AutoCompileContext autoP4TestContext(new P4TestContext);
    auto &options = P4TestContext::get().options();
    const char *argv = "./gtest-p4testgen";
    options.process(1, (char *const *)&argv);
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    std::string includeDir = std::string(buildPath) + std::string("p4include");
    auto *originalEnv = getenv("P4C_16_INCLUDE_PATH");
    setenv("P4C_16_INCLUDE_PATH", includeDir.c_str(), 1);
    options.compilerVersion = P4TEST_VERSION_STRING;
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
    return program->apply(midEnd);
}

bool checkMember(const IR::Expression *expr, const char *name) {
    if (const auto *member = expr->to<IR::Member>()) {
        return member->member.originalName == name;
    }
    return false;
}

bool checkMembers(const IR::Operation_Binary *binOp, const char *lName, const char *rName) {
    return checkMember(binOp->left, lName) && checkMember(binOp->right, rName);
}

const IR::Expression *parseExpression(const char *str, const IR::P4Program *p) {
    CHECK_NULL(str);
    CHECK_NULL(p);
    return P4Tools::ExpressionParser::Parser::getIR(str, p)->to<IR::Expression>();
}

TEST_F(P4ExpressionParserTest, SimpleExpressions) {
    const auto *program =
        loadExample("backends/p4tools/testgen/targets/bmv2/test/p4-programs/bmv2_if.p4");
    // Check local variable
    const auto *expr = parseExpression("ingress::d", program);
    ASSERT_TRUE(expr->is<IR::PathExpression>());
    ASSERT_TRUE(expr->to<IR::PathExpression>()->path->name.originalName == "d");
    ASSERT_TRUE(expr->to<IR::PathExpression>()->path->name.name == "d_0");
    ASSERT_TRUE(expr->type->is<IR::Type_Bits>());
    const auto *type = expr->type->to<IR::Type_Bits>();
    ASSERT_EQ(type->size, 8);

    // Check member.
    expr = parseExpression("egress::h.h.b", program);
    ASSERT_TRUE(checkMember(expr, "b"));
    const auto *member = expr->to<IR::Member>();
    ASSERT_TRUE(member->type->is<IR::Type_Bits>());
    type = member->type->to<IR::Type_Bits>();
    ASSERT_EQ(type->size, 8);
    ASSERT_TRUE(checkMember(member->expr, "h"));
    member = member->expr->to<IR::Member>();
    ASSERT_TRUE(member->type->is<IR::Type_StructLike>());
    const auto *strct = member->type->to<IR::Type_StructLike>();
    ASSERT_EQ(strct->name.originalName, "H");
    ASSERT_TRUE(member->expr->is<IR::PathExpression>());
    const auto *pathExpression = member->expr->to<IR::PathExpression>();
    ASSERT_EQ(pathExpression->path->name.originalName, "h");
    ASSERT_TRUE(pathExpression->type->is<IR::Type_StructLike>());
    strct = pathExpression->type->to<IR::Type_StructLike>();
    ASSERT_EQ(strct->name.originalName, "Headers");

    // Check true/false.
    expr = parseExpression("true", program);
    ASSERT_TRUE(expr->is<IR::BoolLiteral>());
    const auto *boolLiteral = expr->to<IR::BoolLiteral>();
    ASSERT_TRUE(boolLiteral->value);
    expr = parseExpression("false", program);
    ASSERT_TRUE(expr->is<IR::BoolLiteral>());
    boolLiteral = expr->to<IR::BoolLiteral>();
    ASSERT_TRUE(!boolLiteral->value);

    // Check simple constant.
    expr = parseExpression("10", program);
    ASSERT_TRUE(expr->is<IR::Constant>());
    const auto *cnsnt = expr->to<IR::Constant>();
    ASSERT_EQ(cnsnt->value, 10);

    // Check hex constant.
    expr = parseExpression("0xaB", program);
    ASSERT_TRUE(expr->is<IR::Constant>());
    cnsnt = expr->to<IR::Constant>();
    ASSERT_EQ(cnsnt->value, 171);

    // check method call
    expr = parseExpression("ingress::h.h.isValid()", program);
    ASSERT_TRUE(expr->is<IR::MethodCallExpression>());
    const auto *method = expr->to<IR::MethodCallExpression>();
    ASSERT_TRUE(checkMember(method->method, "isValid"));
    member = method->method->to<IR::Member>();
    ASSERT_TRUE(checkMember(member->expr, "h"));
    member = member->expr->to<IR::Member>();
    ASSERT_TRUE(member->type->is<IR::Type_StructLike>());
    strct = member->type->to<IR::Type_StructLike>();
    ASSERT_EQ(strct->name.originalName, "H");
    ASSERT_TRUE(member->expr->is<IR::PathExpression>());
    pathExpression = member->expr->to<IR::PathExpression>();
    ASSERT_EQ(pathExpression->path->name.originalName, "h");
    ASSERT_TRUE(pathExpression->type->is<IR::Type_StructLike>());
    strct = pathExpression->type->to<IR::Type_StructLike>();
    ASSERT_EQ(strct->name.originalName, "Headers");

    // check slice
    expr = parseExpression("ingress::h.h.a[3:2]", program);
    ASSERT_TRUE(expr->type->is<IR::Type_Bits>());
    type = expr->type->to<IR::Type_Bits>();
    ASSERT_EQ(type->size, 2);
    ASSERT_TRUE(expr->is<IR::Slice>());
    const auto *slice = expr->to<IR::Slice>();
    ASSERT_EQ(slice->getL(), 2U);
    ASSERT_EQ(slice->getH(), 3U);
    ASSERT_TRUE(checkMember(slice->e0, "a"));
    member = slice->e0->to<IR::Member>();
    ASSERT_TRUE(member->type->is<IR::Type_Bits>());
    type = member->type->to<IR::Type_Bits>();
    ASSERT_EQ(type->size, 8);

    // check array
    expr = parseExpression("ingress::h.harray[0]", program);
    ASSERT_TRUE(expr->is<IR::ArrayIndex>());
    const auto *array = expr->to<IR::ArrayIndex>();
    ASSERT_TRUE(array->left->is<IR::Member>());
    ASSERT_TRUE(array->left->type->is<IR::Type_Stack>());
    ASSERT_TRUE(array->left->type->to<IR::Type_Stack>()->getSize() == 2);
    ASSERT_TRUE(array->right->is<IR::Constant>());
    ASSERT_TRUE(array->right->to<IR::Constant>()->value == 0);

    // logical not and or, and implication
    expr = parseExpression(
        "!ingress::h.h.b1 && ingress::h.h.b2 || ingress::h.h.b1 -> \
                            ingress::h.h.b2",
        program);
    ASSERT_TRUE(expr->is<IR::LOr>());
    const auto *orExpr = expr->to<IR::LOr>();
    ASSERT_TRUE(checkMember(orExpr->right, "b2"));
    ASSERT_TRUE(orExpr->left->is<IR::LNot>());
    expr = orExpr->left->to<IR::LNot>()->expr;
    ASSERT_TRUE(expr->is<IR::LOr>());
    orExpr = expr->to<IR::LOr>();
    ASSERT_TRUE(checkMember(orExpr->right, "b1"));
    ASSERT_TRUE(orExpr->left->is<IR::LAnd>());
    const auto *andExpr = orExpr->left->to<IR::LAnd>();
    ASSERT_TRUE(checkMember(andExpr->right, "b2"));
    ASSERT_TRUE(andExpr->left->is<IR::LNot>());
    ASSERT_TRUE(checkMember(andExpr->left->to<IR::LNot>()->expr, "b1"));

    // complement
    expr = parseExpression("~ingress::h.h.a", program);
    ASSERT_TRUE(expr->is<IR::Cmpl>());
    const auto *cmpl = expr->to<IR::Cmpl>();
    ASSERT_TRUE(checkMember(cmpl->expr, "a"));

    // mul, div, %
    expr = parseExpression("ingress::h.h.a * ingress::h.h.b / ingress::h.h.c % ingress::h.h.a",
                           program);
    ASSERT_TRUE(expr->is<IR::Div>());
    const auto *divExpr = expr->to<IR::Div>();
    ASSERT_TRUE(divExpr->left->is<IR::Mul>());
    const auto *mltExpr = divExpr->left->to<IR::Mul>();
    ASSERT_TRUE(checkMember(mltExpr->left, "a"));
    ASSERT_TRUE(checkMember(mltExpr->right, "b"));
    ASSERT_TRUE(divExpr->right->is<IR::Mod>());
    const auto *modExpr = divExpr->right->to<IR::Mod>();
    ASSERT_TRUE(checkMember(modExpr->left, "c"));
    ASSERT_TRUE(checkMember(modExpr->right, "a"));

    /// -, |-|, +, |+|
    expr = parseExpression(
        "ingress::h.h.a - ingress::h.h.b |-| ingress::h.h.c + ingress::h.h.a \
                           |+| ingress::h.h.b",
        program);
    ASSERT_TRUE(expr->is<IR::AddSat>());
    const auto *addSat = expr->to<IR::AddSat>();
    ASSERT_TRUE(checkMember(addSat->right, "b"));
    ASSERT_TRUE(addSat->left->is<IR::Add>());
    const auto *addExpr = addSat->left->to<IR::Add>();
    ASSERT_TRUE(checkMember(addExpr->right, "a"));
    ASSERT_TRUE(addExpr->left->is<IR::SubSat>());
    const auto *subSat = addExpr->left->to<IR::SubSat>();
    ASSERT_TRUE(checkMember(subSat->right, "c"));
    ASSERT_TRUE(subSat->left->is<IR::Sub>());
    const auto *subExpr = subSat->left->to<IR::Sub>();
    ASSERT_TRUE(checkMember(subExpr->left, "a"));
    ASSERT_TRUE(checkMember(subExpr->right, "b"));

    // <, <=, >, >=
    expr = parseExpression("ingress::h.h.a < ingress::h.h.b", program);
    ASSERT_TRUE(expr->is<IR::Lss>());
    ASSERT_TRUE(checkMembers(expr->to<IR::Lss>(), "a", "b"));
    expr = parseExpression("ingress::h.h.a <= ingress::h.h.b", program);
    ASSERT_TRUE(expr->is<IR::Leq>());
    ASSERT_TRUE(checkMembers(expr->to<IR::Leq>(), "a", "b"));
    expr = parseExpression("ingress::h.h.a > ingress::h.h.b", program);
    ASSERT_TRUE(expr->is<IR::Grt>());
    ASSERT_TRUE(checkMembers(expr->to<IR::Grt>(), "a", "b"));
    expr = parseExpression("ingress::h.h.a >= ingress::h.h.b", program);
    ASSERT_TRUE(expr->is<IR::Geq>());
    ASSERT_TRUE(checkMembers(expr->to<IR::Geq>(), "a", "b"));

    // ==, !=
    expr = parseExpression("ingress::h.h.a == ingress::h.h.b", program);
    ASSERT_TRUE(expr->is<IR::Equ>());
    ASSERT_TRUE(checkMembers(expr->to<IR::Equ>(), "a", "b"));
    expr = parseExpression("ingress::h.h.a != ingress::h.h.b", program);
    ASSERT_TRUE(expr->is<IR::Neq>());
    ASSERT_TRUE(checkMembers(expr->to<IR::Neq>(), "a", "b"));

    // <<, >>
    expr = parseExpression("ingress::h.h.a << ingress::h.h.b", program);
    ASSERT_TRUE(expr->is<IR::Shl>());
    ASSERT_TRUE(checkMembers(expr->to<IR::Shl>(), "a", "b"));
    expr = parseExpression("ingress::h.h.a >> ingress::h.h.b", program);
    ASSERT_TRUE(expr->is<IR::Shr>());
    ASSERT_TRUE(checkMembers(expr->to<IR::Shr>(), "a", "b"));

    // (a + b) * c
    expr = parseExpression("(ingress::h.h.a + ingress::h.h.b) * ingress::h.h.c", program);
    ASSERT_TRUE(expr->is<IR::Mul>());
    mltExpr = expr->to<IR::Mul>();
    ASSERT_TRUE(checkMember(mltExpr->right, "c"));
    ASSERT_TRUE(mltExpr->left->is<IR::Add>());
    ASSERT_TRUE(checkMembers(mltExpr->left->to<IR::Add>(), "a", "b"));

    // a + b * c
    expr = parseExpression("ingress::h.h.a + ingress::h.h.b * ingress::h.h.c", program);
    ASSERT_TRUE(expr->is<IR::Add>());
    addExpr = expr->to<IR::Add>();
    ASSERT_TRUE(checkMember(addExpr->left, "a"));
    ASSERT_TRUE(addExpr->right->is<IR::Mul>());
    ASSERT_TRUE(checkMembers(addExpr->right->to<IR::Mul>(), "b", "c"));

    // a - b - c
    expr = parseExpression("ingress::h.h.a - ingress::h.h.b - ingress::h.h.c", program);
    ASSERT_TRUE(expr->is<IR::Sub>());
    subExpr = expr->to<IR::Sub>();
    ASSERT_TRUE(checkMember(subExpr->right, "c"));
    ASSERT_TRUE(subExpr->left->is<IR::Sub>());
    ASSERT_TRUE(checkMembers(subExpr->left->to<IR::Sub>(), "a", "b"));

    // (a - b) - c
    expr = parseExpression("(ingress::h.h.a - ingress::h.h.b) - ingress::h.h.c", program);
    ASSERT_TRUE(expr->is<IR::Sub>());
    subExpr = expr->to<IR::Sub>();
    ASSERT_TRUE(checkMember(subExpr->right, "c"));
    ASSERT_TRUE(subExpr->left->is<IR::Sub>());
    ASSERT_TRUE(checkMembers(subExpr->left->to<IR::Sub>(), "a", "b"));

    // a - (b - c)
    expr = parseExpression("ingress::h.h.a - (ingress::h.h.b - ingress::h.h.c)", program);
    ASSERT_TRUE(expr->is<IR::Sub>());
    subExpr = expr->to<IR::Sub>();
    ASSERT_TRUE(checkMember(subExpr->left, "a"));
    ASSERT_TRUE(subExpr->right->is<IR::Sub>());
    ASSERT_TRUE(checkMembers(subExpr->right->to<IR::Sub>(), "b", "c"));

    // -a
    expr = parseExpression("-ingress::h.h.a", program);
    ASSERT_TRUE(expr->is<IR::Mul>());
    mltExpr = expr->to<IR::Mul>();
    ASSERT_TRUE(mltExpr->left->is<IR::Constant>());
    ASSERT_TRUE(mltExpr->left->to<IR::Constant>()->value == -1);
    ASSERT_TRUE(checkMember(mltExpr->right, "a"));

    // +a
    expr = parseExpression("+ingress::h.h.a", program);
    ASSERT_TRUE(checkMember(expr, "a"));

    // -(a + b)
    expr = parseExpression("-(ingress::h.h.a + ingress::h.h.b)", program);
    ASSERT_TRUE(expr->is<IR::Mul>());
    mltExpr = expr->to<IR::Mul>();
    ASSERT_TRUE(mltExpr->left->is<IR::Constant>());
    ASSERT_TRUE(mltExpr->left->to<IR::Constant>()->value == -1);
    ASSERT_TRUE(mltExpr->right->is<IR::Add>());
    ASSERT_TRUE(checkMembers(mltExpr->right->to<IR::Add>(), "a", "b"));

    // +(a + b)
    expr = parseExpression("+(ingress::h.h.a + ingress::h.h.b)", program);
    ASSERT_TRUE(expr->is<IR::Add>());
    ASSERT_TRUE(checkMembers(expr->to<IR::Add>(), "a", "b"));

    // !(b1||b2)
    expr = parseExpression("!(ingress::h.h.b1 || ingress::h.h.b2)", program);
    ASSERT_TRUE(expr->is<IR::LNot>());
    const auto *lNot = expr->to<IR::LNot>();
    ASSERT_TRUE(lNot->expr->is<IR::LOr>());
    ASSERT_TRUE(checkMembers(lNot->expr->to<IR::LOr>(), "b1", "b2"));

    // d == (bit<4>)a
    expr = parseExpression("ingress::h.h.d == (bit<4>)ingress::h.h.a", program);
    ASSERT_TRUE(expr->is<IR::Equ>());
    const auto *equ = expr->to<IR::Equ>();
    ASSERT_TRUE(checkMember(equ->left, "d"));
    ASSERT_TRUE(equ->right->is<IR::Cast>());
    const auto *cast = equ->right->to<IR::Cast>();
    ASSERT_TRUE(cast->destType->is<IR::Type_Bits>());
    type = equ->right->type->to<IR::Type_Bits>();
    ASSERT_EQ(type->width_bits(), 4);
    ASSERT_TRUE(checkMember(cast->expr, "a"));
}

}  // namespace Test
