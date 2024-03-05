#include "backends/p4tools/common/core/z3_solver.h"

#include <z3++.h>
#include <z3_api.h>

#include <algorithm>
#include <cstdint>
#include <exception>
#include <iterator>
#include <map>
#include <string>
#include <utility>

#include <boost/multiprecision/cpp_int.hpp>

#include "ir/ir.h"
#include "ir/irutils.h"
#include "ir/json_loader.h"  // IWYU pragma: keep
#include "ir/json_parser.h"  // IWYU pragma: keep
#include "ir/node.h"
#include "ir/visitor.h"
#include "lib/big_int_util.h"
#include "lib/exceptions.h"
#include "lib/indent.h"
#include "lib/log.h"
#include "lib/timer.h"

namespace P4Tools {

/// Converts a Z3 expression to a string.
const char *toString(const z3::expr &e) { return Z3_ast_to_string(e.ctx(), e); }

#ifndef NDEBUG
template <typename... Args>
std::string stringFormat(const char *format, Args... args) {
    size_t size = snprintf(nullptr, 0, format, args...) + 1;
    BUG_CHECK(size > 0, "Z3Solver: error during formatting.");
    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, format, args...);
    return {buf.get(), buf.get() + size - 1};
}

#define Z3_LOG(FORMAT, ...)                                                                       \
    LOG1(stringFormat("Z3Solver:%s() in %s, line %i: " FORMAT "\n", __func__, __FILE__, __LINE__, \
                      __VA_ARGS__))

/// Converts a Z3 model to a string.
const char *toString(z3::model m) { return Z3_model_to_string(m.ctx(), m); }
#else
#define Z3_LOG(...)
#endif  // NDEBUG

z3::sort Z3Solver::toSort(const IR::Type *type) {
    BUG_CHECK(type, "Z3Solver::toSort with empty pointer");

    if (type->is<IR::Type_Boolean>()) {
        return ctx().bool_sort();
    }

    if (const auto *bits = type->to<IR::Type_Bits>()) {
        return ctx().bv_sort(bits->width_bits());
    }

    if (type->is<IR::Type_String>()) {
        return ctx().string_sort();
    }

    BUG("Z3Solver: unimplemented type %1%: %2% ", type->node_type_name(), type);
}

std::string Z3Solver::generateName(const IR::SymbolicVariable &var) {
    std::ostringstream ostr;
    ostr << var;
    return ostr.str();
}

z3::expr Z3Solver::declareVar(const IR::SymbolicVariable &var) {
    auto sort = toSort(var.type);
    auto expr = ctx().constant(generateName(var).c_str(), sort);
    BUG_CHECK(
        !declaredVarsById.empty(),
        "DeclaredVarsById should have at least one entry! Check if push() was used correctly.");
    // Need to take the reference here to avoid accidental copies.
    auto *latestVars = &declaredVarsById.back();
    latestVars->emplace(expr.id(), &var);
    return expr;
}

void Z3Solver::reset() {
    z3solver.reset();
    declaredVarsById.clear();
    checkpoints.clear();
    z3Assertions.resize(0);
}

void Z3Solver::clearMemory() {
    auto p4AssertionsBuf = p4Assertions;
    reset();
    Z3_finalize_memory();
    z3solver = z3::solver(*new z3::context());
    p4Assertions.clear();
    for (const auto &assert : p4AssertionsBuf) {
        push();
        asrt(assert);
    }
}

void Z3Solver::push() {
    if (isIncremental) {
        z3solver.push();
    }
    checkpoints.push_back(p4Assertions.size());
    declaredVarsById.emplace_back();
}

void Z3Solver::pop() {
    BUG_CHECK(isIncremental || z3Assertions.size() == p4Assertions.size(),
              "Number of assertions in P4 and Z3 formats aren't equal");
    BUG_CHECK(!checkpoints.empty(), "Check points list is empty");

    size_t sz = checkpoints.back();
    checkpoints.pop_back();
    // TODO: This check should be active, but because of JSON loader issues we can not use it.
    // So we have to be tolerant for now.
    // BUG_CHECK(!declaredVarsById.empty(), "Declaration list is empty");
    if (!declaredVarsById.empty()) {
        declaredVarsById.pop_back();
    }
    if (isIncremental) {
        z3solver.pop();
    } else {
        z3Assertions.resize(sz);
        if (seed_) {
            seed(*seed_);
        }
        if (timeout_) {
            timeout(*timeout_);
        }
    }

    // Update p4Assertions to match.
    BUG_CHECK(p4Assertions.size() >= sz, "Invalid size of assertions");
    p4Assertions.resize(sz);
}

void Z3Solver::comment(cstring commentStr) {
    // GCC complains about an unused parameter here.
    (void)(commentStr);
    Z3_LOG("additional comment:%s", commentStr);
}

void Z3Solver::seed(unsigned seed) {
    Z3_LOG("set a new seed:'%d'", seed);
    auto &z3context = z3solver.ctx();
    z3::params param(z3context);
    param.set("phase_selection", 5U);
    param.set("random_seed", seed);
    z3solver.set(param);
    seed_ = seed;
}

void Z3Solver::timeout(unsigned tm) {
    Z3_LOG("set a timeout:'%d'", tm);
    auto &z3context = z3solver.ctx();
    z3::params param(z3context);
    param.set(":timeout", tm);
    z3solver.set(param);
    timeout_ = tm;
}

std::optional<bool> Z3Solver::interpretSolverResult(z3::check_result result) {
    switch (result) {
        case z3::sat:
            Z3_LOG("result:%s", "sat");
            return true;
        case z3::unsat:
            Z3_LOG("result:%s", "unsat");
            return false;

        default:  // unknown
            Z3_LOG("result:%s", "unknown");
            return std::nullopt;
    }
}

std::optional<bool> Z3Solver::checkSat() {
    Util::ScopedTimer ctCheckSat("checkSat");
    return interpretSolverResult(z3solver.check());
}

std::optional<bool> Z3Solver::checkSat(const z3::expr_vector &asserts) {
    Util::ScopedTimer ctCheckSat("checkSat");
    return interpretSolverResult(z3solver.check(asserts));
}

std::optional<bool> Z3Solver::checkSat(const std::vector<const Constraint *> &asserts) {
    Util::ScopedTimer ctZ3("z3");
    if (isIncremental) {
        // Find common prefix with the previous invocation's list of assertions
        auto from = asserts.begin();
        auto to = asserts.begin() + std::min(p4Assertions.size(), asserts.size());
        from = std::mismatch(from, to, p4Assertions.begin()).first;
        size_t commonPrefixLen = std::distance(asserts.begin(), from);
        // and pop all assertions past the common prefix.
        while (p4Assertions.size() > commonPrefixLen) {
            pop();
        }
    } else {
        reset();
    }
    // Push all assertions after (including) the first assertion which differs since the last
    // invocation (or all in case of nonincremental mode).
    for (size_t i = p4Assertions.size(); i < asserts.size(); ++i) {
        push();
        asrt(asserts[i]);
    }
    Z3_LOG("checking satisfiability for %d assertions",
           isIncremental ? z3solver.assertions().size() : z3Assertions.size());
    return isIncremental ? checkSat() : checkSat(z3Assertions);
}

void Z3Solver::asrt(const Constraint *assertion) {
    CHECK_NULL(assertion);
    Z3Translator z3translator(*this);
    auto expr = z3translator.translate(assertion);
    asrt(expr);
    p4Assertions.push_back(assertion);
    BUG_CHECK(isIncremental || z3Assertions.size() == p4Assertions.size(),
              "Number of assertion in P4 and Z3 formats aren't equal");
}

void Z3Solver::asrt(const z3::expr &assertion) {
    try {
        Z3_LOG("add assertion '%s'", toString(assertion));
        if (isIncremental) {
            z3solver.add(assertion);
        } else {
            z3Assertions.push_back(assertion);
        }
    } catch (z3::exception &e) {
        BUG("Z3Solver: Z3 exception: %1%\nAssertion %2%", e.msg(), assertion);
    }
}

const SymbolicMapping &Z3Solver::getSymbolicMapping() const {
    Util::ScopedTimer ctZ3("z3");
    auto *result = new SymbolicMapping();
    // First, collect a map of all the declared variables we have encountered in the stack.
    std::map<unsigned int, const IR::SymbolicVariable *> declaredVars;
    for (auto it = declaredVarsById.rbegin(); it != declaredVarsById.rend(); ++it) {
        auto latestVars = *it;
        for (auto var : latestVars) {
            declaredVars.emplace(var);
        }
    }
    // Then, get the model and match each declaration in the model to its IR::SymbolicVariable.
    try {
        Util::ScopedTimer ctCheckSat("getModel");
        auto z3Model = z3solver.get_model();
        Z3_LOG("z3 model:%s", toString(z3Model));

        // Loop through each declaration in the Z3 model and convert to the output model.
        for (size_t i = 0; i < z3Model.size(); i++) {
            auto z3Func = z3Model[i];
            BUG_CHECK(z3Func.arity() == 0, "Z3Solver: invalid format of variable in getModel");

            // Get the Z3 expression and value for the current declaration.
            auto z3Expr = z3Func();
            auto z3Value = z3Model.get_const_interp(z3Func);

            // Convert to a symbolic variable and value.
            auto exprId = z3Expr.id();
            BUG_CHECK(declaredVars.count(exprId) > 0, "Z3Solver: unknown variable declaration: %1%",
                      z3Expr);
            const auto *symbolicVar = declaredVars.at(exprId);
            const auto *value = toLiteral(z3Value, symbolicVar->type);
            result->emplace(symbolicVar, value);
        }
    } catch (z3::exception &e) {
        BUG("Z3Solver : Z3 exception: %1%", e.msg());
    } catch (std::exception &exception) {
        BUG("Z3Solver : can't get a model from z3: %1%", exception.what());
    } catch (...) {
        BUG("Z3Solver : unknown segmentation fault in getModel");
    }
    return *result;
}

const IR::Literal *Z3Solver::toLiteral(const z3::expr &e, const IR::Type *type) {
    // Handle booleans.
    if (type->is<IR::Type::Boolean>()) {
        BUG_CHECK(e.is_bool(), "Expected a boolean value: %1%", e);
        return new IR::BoolLiteral(type, e.is_true());
    }

    // Handle bit vectors.
    const auto *bitsType = type->to<IR::Type::Bits>();
    BUG_CHECK(bitsType, "Type %1% is not bit<n>", type);
    BUG_CHECK(e.is_bv(), "Expected a bit vector: %1%", e);
    std::string strNum = toString(z3::bv2int(e, bitsType->isSigned).simplify());
    // Z3 prints negative numbers in the following syntax: (- number).
    // For big_int constructor brackets and spaces should be eliminated.
    if (strNum.find("(-") == 0) {
        strNum.erase(0, 1);
        strNum.erase(strNum.length() - 1, 1);
        strNum.erase(remove(strNum.begin(), strNum.end(), ' '), strNum.end());
    }
    big_int bigint(strNum.c_str());
    return IR::getConstant(type, bigint);
}

void Z3Solver::toJSON(JSONGenerator &json) const {
    json << json.indent << "{\n";
    json.indent++;
    json << json.indent << "\"checkpoints\" : " << checkpoints;
    json << ",\n";
    json << json.indent << "\"declarations\" : " << declaredVarsById;
    json << ",\n";
    json << json.indent << "\"assertions\" : " << p4Assertions;
    json.indent--;
    json << json.indent << "}\n";
}

void Z3Solver::addZ3Pushes(size_t &chkIndex, size_t asrtIndex) {
    // Only add push commands to Z3 in incremental mode.
    if (!isIncremental) {
        return;
    }

    for (; chkIndex < checkpoints.size(); chkIndex++) {
        if (checkpoints[chkIndex] != asrtIndex) {
            return;
        }
        z3solver.push();
    }
}

const z3::solver &Z3Solver::getZ3Solver() const { return z3solver; }

const z3::context &Z3Solver::getZ3Ctx() const { return z3solver.ctx(); }

z3::context &Z3Solver::ctx() const { return z3solver.ctx(); }

bool Z3Solver::isInIncrementalMode() const { return isIncremental; }

Z3Solver::Z3Solver(bool isIncremental, std::optional<std::istream *> inOpt)
    : z3solver(*new z3::context), isIncremental(isIncremental), z3Assertions(ctx()) {
    // Add a top-level set to declaration vars that we can insert variables.
    // TODO: Think about whether this is necessary or it is not better to remove it.
    declaredVarsById.emplace_back();
    if (inOpt == std::nullopt) {
        return;
    }

    JSONLoader loader(*inOpt.value());

    JSONLoader solverCheckpoints(loader, "checkpoints");
    BUG_CHECK(solverCheckpoints.json->is<JsonVector>(),
              "Z3 solver loading: can't find list of checkpoint");
    solverCheckpoints >> checkpoints;

    // loading all assertions
    JSONLoader solverAssertions(loader, "assertions");
    BUG_CHECK(solverAssertions.json->is<JsonVector>(),
              "Z3 solver loading: can't find list of assertions");
    safe_vector<const Constraint *> assertions;
    solverAssertions >> assertions;
    size_t chkIndex = 0;
    for (size_t i = 0; i < assertions.size(); i++) {
        addZ3Pushes(chkIndex, i);
        asrt(assertions[i]);
    }
    addZ3Pushes(chkIndex, assertions.size());
}

Z3Translator::Z3Translator(Z3Solver &solver) : result(solver.ctx()), solver(solver) {}

bool Z3Translator::preorder(const IR::Node *node) {
    BUG("%1%: Unhandled node type: %2%", node, node->node_type_name());
}

bool Z3Translator::preorder(const IR::Cast *cast) {
    Z3Translator tCast(solver);
    cast->expr->apply(tCast);
    uint64_t exprSize = 0;
    const auto *const castExtrType = cast->expr->type;
    auto castExpr = tCast.result;
    if (const auto *tb = cast->destType->to<IR::Type_Bits>()) {
        uint64_t destSize = tb->width_bits();
        if (const auto *exprType = castExtrType->to<IR::Type_Bits>()) {
            exprSize = exprType->width_bits();
        } else if (castExtrType->is<IR::Type_Boolean>()) {
            exprSize = 1;
            auto trueVal = solver.get().ctx().bv_val(1, exprSize);
            auto falseVal = solver.get().ctx().bv_val(0, exprSize);
            castExpr = z3::ite(castExpr, trueVal, falseVal);
        } else if (const auto *exprType = castExtrType->to<IR::Extracted_Varbits>()) {
            exprSize = exprType->width_bits();
        } else {
            BUG("Casting %s to a bit vector is not supported.", castExpr.to_string().c_str());
        }
        // At this point we are only dealing with expr bit vectors.
        if (exprSize < destSize) {
            // The target value is larger, extend with zeros.
            result = z3::zext(castExpr, destSize - exprSize);
        }
        if (exprSize > destSize) {
            // The target value is smaller, truncate everything on the right.
            result = castExpr.extract(destSize - 1, 0);
        }
        if (exprSize == destSize) {
            result = castExpr;
        }
        return false;
    }
    if (cast->destType->is<IR::Type_Boolean>()) {
        if (const auto *exprType = castExtrType->to<IR::Type_Bits>()) {
            if (exprType->width_bits() == 1) {
                castExpr = z3::operator==(castExpr, solver.get().ctx().bv_val(1, 1));
            } else {
                BUG("Cast expression type %1% is not bit<1> : %2%", exprType, castExpr);
            }
        } else if (castExtrType->is<IR::Type_Boolean>()) {
            result = castExpr;
        } else {
            BUG("Casting %s to a bool is not supported.", castExpr.to_string().c_str());
        }
        result = castExpr;
        return false;
    }
    BUG("%1%: Unhandled cast type: %2%", cast, cast->node_type_name());
}

bool Z3Translator::preorder(const IR::Constant *constant) {
    // Handle infinite-integer constants.
    if (constant->type->is<IR::Type_InfInt>()) {
        result = solver.get().ctx().int_val(constant->value.str().c_str());
        return false;
    }

    // Handle bit<n> constants.
    if (const auto *bits = constant->type->to<IR::Type_Bits>()) {
        result = solver.get().ctx().bv_val(constant->value.str().c_str(), bits->size);
        return false;
    }

    if (const auto *bits = constant->type->to<IR::Extracted_Varbits>()) {
        result = solver.get().ctx().bv_val(constant->value.str().c_str(), bits->width_bits());
        return false;
    }

    BUG("Z3Translator: unsupported type for constant %1%", constant);
}

bool Z3Translator::preorder(const IR::BoolLiteral *boolLiteral) {
    result = solver.get().ctx().bool_val(boolLiteral->value);
    return false;
}

bool Z3Translator::preorder(const IR::StringLiteral *stringLiteral) {
    result = solver.get().ctx().string_val(stringLiteral->value);
    return false;
}

bool Z3Translator::preorder(const IR::SymbolicVariable *var) {
    result = solver.get().declareVar(*var);
    return false;
}

bool Z3Translator::preorder(const IR::Neg *op) { return recurseUnary(op, z3::operator-); }

bool Z3Translator::preorder(const IR::Cmpl *op) { return recurseUnary(op, z3::operator~); }

bool Z3Translator::preorder(const IR::LNot *op) { return recurseUnary(op, z3::operator!); }

template <class ShiftType>
const ShiftType *Z3Translator::rewriteShift(const ShiftType *shift) const {
    BUG_CHECK(shift->template is<IR::Shl>() || shift->template is<IR::Shr>(),
              "Not a shift operation: %1%", shift);

    const IR::Expression *left = shift->left;
    const IR::Expression *right = shift->right;

    // See if we are shifting a bit vector by an infinite-precision integer. If we aren't, then
    // there's nothing to do.
    if (!left->type->is<IR::Type::Bits>() || !right->type->is<IR::Type_InfInt>()) {
        return shift;
    }

    // The infinite-precision integer should be a compile-time known constant. Rewrite it as a bit
    // vector.
    const auto *shiftAmount = right->to<IR::Constant>();
    BUG_CHECK(shiftAmount, "Shift amount is not a compile-time known constant: %1%", right);
    const auto *newShiftAmount = IR::getConstant(left->type, shiftAmount->value);

    return new ShiftType(shift->type, left, newShiftAmount);
}

/// General function for unary operations.
bool Z3Translator::recurseUnary(const IR::Operation_Unary *unary, Z3UnaryOp f) {
    BUG_CHECK(unary, "Z3Translator: encountered null node during translation");
    Z3Translator tExpr(solver);
    unary->expr->apply(tExpr);
    result = f(tExpr.result);
    return false;
}

bool Z3Translator::preorder(const IR::Equ *op) { return recurseBinary(op, z3::operator==); }

bool Z3Translator::preorder(const IR::Neq *op) { return recurseBinary(op, z3::operator!=); }

bool Z3Translator::preorder(const IR::Lss *op) {
    const auto *leftType = op->left->type->to<IR::Type::Bits>();
    if (leftType != nullptr && !leftType->isSigned) {
        return recurseBinary(op, z3::ult);
    }
    return recurseBinary(op, z3::operator<);
}

bool Z3Translator::preorder(const IR::Leq *op) {
    const auto *leftType = op->left->type->to<IR::Type::Bits>();
    if (leftType != nullptr && !leftType->isSigned) {
        return recurseBinary(op, z3::ule);
    }
    return recurseBinary(op, z3::operator<=);
}

bool Z3Translator::preorder(const IR::Grt *op) {
    const auto *leftType = op->left->type->to<IR::Type::Bits>();
    if (leftType != nullptr && !leftType->isSigned) {
        return recurseBinary(op, z3::ugt);
    }
    return recurseBinary(op, z3::operator>);
}

bool Z3Translator::preorder(const IR::Geq *op) {
    const auto *leftType = op->left->type->to<IR::Type::Bits>();
    if (leftType != nullptr && !leftType->isSigned) {
        return recurseBinary(op, z3::uge);
    }
    return recurseBinary(op, z3::operator>=);
}

bool Z3Translator::preorder(const IR::Mod *op) { return recurseBinary(op, z3::operator%); }

bool Z3Translator::preorder(const IR::Add *op) { return recurseBinary(op, z3::operator+); }

bool Z3Translator::preorder(const IR::Sub *op) { return recurseBinary(op, z3::operator-); }

bool Z3Translator::preorder(const IR::Mul *op) { return recurseBinary(op, z3::operator*); }

bool Z3Translator::preorder(const IR::Div *op) { return recurseBinary(op, z3::operator/); }

bool Z3Translator::preorder(const IR::Shl *op) {
    op = rewriteShift(op);
    return recurseBinary(op, z3::shl);
}

bool Z3Translator::preorder(const IR::Shr *op) {
    op = rewriteShift(op);

    // Do an arithmetic shift if the left operand is signed; otherwise, do a logical shift.
    const auto *leftType = op->left->type->to<IR::Type::Bits>();
    BUG_CHECK(leftType, "Shift operand is not a bits<n>: %1%", op->left);

    if (leftType->isSigned) {
        return recurseBinary(op, z3::ashr);
    }
    return recurseBinary(op, z3::lshr);
}

bool Z3Translator::preorder(const IR::BAnd *op) { return recurseBinary(op, z3::operator&); }

bool Z3Translator::preorder(const IR::BOr *op) { return recurseBinary(op, z3::operator|); }

bool Z3Translator::preorder(const IR::BXor *op) { return recurseBinary(op, z3::operator^); }

bool Z3Translator::preorder(const IR::LAnd *op) { return recurseBinary(op, z3::operator&&); }

bool Z3Translator::preorder(const IR::LOr *op) { return recurseBinary(op, z3::operator||); }

bool Z3Translator::preorder(const IR::Concat *op) { return recurseBinary(op, z3::concat); }

/// general function for binary operations
bool Z3Translator::recurseBinary(const IR::Operation_Binary *binary, Z3BinaryOp f) {
    BUG_CHECK(binary, "Z3Translator: encountered null node during translation");
    Z3Translator tLeft(solver);
    Z3Translator tRight(solver);
    binary->left->apply(tLeft);
    binary->right->apply(tRight);
    result = f(tLeft.result, tRight.result);
    return false;
}

/// Building ternary operations
bool Z3Translator::preorder(const IR::Mux *op) { return recurseTernary(op, z3::ite); }

bool Z3Translator::preorder(const IR::Slice *op) {
    return recurseTernary(op, [](const z3::expr &e0, const z3::expr &e1, const z3::expr &e2) {
        return e0.extract(e1.simplify().get_numeral_uint(), e2.simplify().get_numeral_uint());
    });
}

/// general function for ternary operations
bool Z3Translator::recurseTernary(const IR::Operation_Ternary *ternary, Z3TernaryOp f) {
    BUG_CHECK(ternary, "Z3Translator: encountered null node during translation");
    Z3Translator t0(solver);
    Z3Translator t1(solver);
    Z3Translator t2(solver);
    ternary->e0->apply(t0);
    ternary->e1->apply(t1);
    ternary->e2->apply(t2);
    result = f(t0.result, t1.result, t2.result);
    return false;
}

z3::expr Z3Translator::getResult() { return result; }

z3::expr Z3Translator::translate(const IR::Expression *expression) {
    try {
        expression->apply(*this);
    } catch (z3::exception &e) {
        BUG("Z3Translator: Z3 exception: %1%\nExpression %2%", e.msg(), expression);
    }
    return result;
}

}  // namespace P4Tools
