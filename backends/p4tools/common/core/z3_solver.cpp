#include "backends/p4tools/common/core/z3_solver.h"

#include <z3_api.h>

#include <algorithm>
#include <cstdint>
#include <exception>
#include <iterator>
#include <list>
#include <map>
#include <ostream>
#include <string>
#include <utility>

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/common/lib/model.h"
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

/// Translates P4 expressions into Z3. Any variables encountered are declared to a Z3 instance.
class Z3Translator : public virtual Inspector {
 public:
    /// Creates a Z3 translator. Any variables encountered during translation will be declared to
    /// the Z3 instance encapsulated within the given solver.
    explicit Z3Translator(Z3Solver &solver);

    /// Handles unexpected nodes.
    bool preorder(const IR::Node *node) override;

    /// Translates casts.
    bool preorder(const IR::Cast *cast) override;

    /// Translates constants.
    bool preorder(const IR::Constant *constant) override;
    bool preorder(const IR::BoolLiteral *boolLiteral) override;

    /// Translates variables.
    bool preorder(const IR::Member *member) override;
    bool preorder(const IR::ConcolicVariable *var) override;

    // Translations for unary operations.
    bool preorder(const IR::Neg *op) override;
    bool preorder(const IR::Cmpl *op) override;
    bool preorder(const IR::LNot *op) override;

    // Translations for binary operations.
    bool preorder(const IR::Equ *op) override;
    bool preorder(const IR::Neq *op) override;
    bool preorder(const IR::Lss *op) override;
    bool preorder(const IR::Leq *op) override;
    bool preorder(const IR::Grt *op) override;
    bool preorder(const IR::Geq *op) override;
    bool preorder(const IR::Mod *op) override;
    bool preorder(const IR::Add *op) override;
    bool preorder(const IR::Sub *op) override;
    bool preorder(const IR::Mul *op) override;
    bool preorder(const IR::Div *op) override;
    bool preorder(const IR::Shl *op) override;
    bool preorder(const IR::Shr *op) override;
    bool preorder(const IR::BAnd *op) override;
    bool preorder(const IR::BOr *op) override;
    bool preorder(const IR::BXor *op) override;
    bool preorder(const IR::LAnd *op) override;
    bool preorder(const IR::LOr *op) override;
    bool preorder(const IR::Concat *op) override;

    // Translations for ternary operations.
    bool preorder(const IR::Mux *op) override;
    bool preorder(const IR::Slice *op) override;

    /// @returns the result of the translation.
    z3::expr getResult() { return result; }

 private:
    /// Function type for a unary operator.
    using Z3UnaryOp = z3::expr (*)(const z3::expr &);

    /// Function type for a binary operator.
    using Z3BinaryOp = z3::expr (*)(const z3::expr &, const z3::expr &);

    /// Function type for a ternary operator.
    using Z3TernaryOp = z3::expr (*)(const z3::expr &, const z3::expr &, const z3::expr &);

    /// Handles recursion into unary operations.
    ///
    /// @returns false.
    bool recurseUnary(const IR::Operation_Unary *unary, Z3UnaryOp f);

    /// Handles recursion into binary operations.
    ///
    /// @returns false.
    bool recurseBinary(const IR::Operation_Binary *binary, Z3BinaryOp f);

    /// Handles recursion into ternary operations.
    ///
    /// @returns false.
    bool recurseTernary(const IR::Operation_Ternary *ternary, Z3TernaryOp f);

    /// Rewrites a shift operation so that the type of the shift amount matches that of the number
    /// being shifted.
    ///
    /// P4 allows shift operands to have different types: when the number being shifted is a bit
    /// vector, the shift amount can be an infinite-precision integer. This rewrites such
    /// expressions so that the shift amount is a bit vector.
    template <class ShiftType>
    const ShiftType *rewriteShift(const ShiftType *shift) const;

    /// The output of the translation.
    z3::expr result;

    /// The Z3 solver instance, to which variables will be declared as they are encountered.
    Z3Solver &solver;
};

z3::sort Z3Solver::toSort(const IR::Type *type) {
    BUG_CHECK(type, "Z3Solver::toSort with empty pointer");

    if (type->is<IR::Type_Boolean>()) {
        return z3context.bool_sort();
    }

    if (const auto *bits = type->to<IR::Type_Bits>()) {
        return z3context.bv_sort(bits->width_bits());
    }

    BUG("Z3Solver: unimplemented type %1%: %2% ", type->node_type_name(), type);
}

std::string Z3Solver::generateName(const IR::StateVariable &var) const {
    std::ostringstream ostr;
    generateName(ostr, var);
    return ostr.str();
}

void Z3Solver::generateName(std::ostringstream &ostr, const IR::StateVariable &var) const {
    // Recurse into the parent member expression to retrieve the full name of the variable.
    if (const auto *next = var->expr->to<IR::Member>()) {
        generateName(ostr, next);
    } else {
        const auto *path = var->expr->to<IR::PathExpression>();
        BUG_CHECK(path, "Z3Solver: expected to be a PathExpression: %1%", var->expr);
        ostr << path->path->name;
    }

    ostr << "." << var->member;
}

z3::expr Z3Solver::declareVar(const IR::StateVariable &var) {
    BUG_CHECK(var, "Z3Solver: attempted to declare a non-member: %1%", var);
    auto sort = toSort(var->type);
    auto expr = z3context.constant(generateName(var).c_str(), sort);
    BUG_CHECK(
        !declaredVarsById.empty(),
        "DeclaredVarsById should have at least one entry! Check if push() was used correctly.");
    // Need to take the reference here to avoid accidental copies.
    auto *latestVars = &declaredVarsById.back();
    latestVars->emplace(expr.id(), var);
    return expr;
}

void Z3Solver::reset() {
    z3solver.reset();
    declaredVarsById.clear();
    checkpoints.clear();
    z3Assertions.resize(0);
}

void Z3Solver::push() {
    if (isIncremental) {
        z3solver.push();
    }
    checkpoints.push_back(p4Assertions.size());
    declaredVarsById.push_back({});
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
    z3::params param(z3context);
    param.set("phase_selection", 5U);
    param.set("random_seed", seed);
    z3solver.set(param);
    seed_ = seed;
}

void Z3Solver::timeout(unsigned tm) {
    Z3_LOG("set a timeout:'%d'", tm);
    z3::params param(z3context);
    param.set(":timeout", tm);
    z3solver.set(param);
    timeout_ = tm;
}

std::optional<bool> Z3Solver::checkSat(const std::vector<const Constraint *> &asserts) {
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
    Util::ScopedTimer ctZ3("z3");
    Util::ScopedTimer ctCheckSat("checkSat");
    z3::check_result result = isIncremental ? z3solver.check() : z3solver.check(z3Assertions);
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

void Z3Solver::asrt(const Constraint *assertion) {
    try {
        Z3Translator z3translator(*this);
        assertion->apply(z3translator);
        auto expr = z3translator.getResult();

        Z3_LOG("add assertion '%s'", toString(expr));
        if (isIncremental) {
            z3solver.add(expr);
        } else {
            z3Assertions.push_back(expr);
        }
        p4Assertions.push_back(assertion);
        BUG_CHECK(isIncremental || z3Assertions.size() == p4Assertions.size(),
                  "Number of assertion in P4 and Z3 formats aren't equal");
    } catch (z3::exception &e) {
        BUG("Z3Solver: Z3 exception: %1%\nAssertion %2%", e.msg(), assertion);
    }
}

const Model *Z3Solver::getModel() const {
    auto *result = new Model();
    // First, collect a map of all the declared variables we have encountered in the stack.
    std::map<unsigned int, IR::StateVariable> declaredVars;
    for (auto it = declaredVarsById.rbegin(); it != declaredVarsById.rend(); ++it) {
        auto latestVars = *it;
        for (auto var : latestVars) {
            declaredVars.emplace(var);
        }
    }
    // Then, get the model and match each declaration in the model to its IR::StateVariable.
    try {
        Util::ScopedTimer ctZ3("z3");
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

            // Convert to a state variable and value.
            auto exprId = z3Expr.id();
            BUG_CHECK(declaredVars.count(exprId) > 0, "Z3Solver: unknown variable declaration: %1%",
                      z3Expr);
            const auto stateVar = declaredVars.at(exprId);
            const auto *value = toLiteral(z3Value, stateVar->type);
            result->emplace(stateVar, value);
        }
    } catch (z3::exception &e) {
        BUG("Z3Solver : Z3 exception: %1%", e.msg());
    } catch (std::exception &exception) {
        BUG("Z3Solver : can't get a model from z3: %1%", exception.what());
    } catch (...) {
        BUG("Z3Solver : unknown segmentation fault in getModel");
    }
    return result;
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

const z3::context &Z3Solver::getZ3Ctx() const { return z3context; }

bool Z3Solver::isInIncrementalMode() const { return isIncremental; }

Z3Solver::Z3Solver(bool isIncremental, std::optional<std::istream *> inOpt)
    : z3solver(z3context), isIncremental(isIncremental), z3Assertions(z3context) {
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

Z3Translator::Z3Translator(Z3Solver &solver) : result(solver.z3context), solver(solver) {}

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
            auto trueVal = solver.z3context.bv_val(1, exprSize);
            auto falseVal = solver.z3context.bv_val(0, exprSize);
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
                castExpr = solver.z3context.bool_val(castExpr.bool_value() == Z3_L_TRUE);
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
        result = solver.z3context.int_val(constant->value.str().c_str());
        return false;
    }

    // Handle bit<n> constants.
    if (const auto *bits = constant->type->to<IR::Type_Bits>()) {
        result = solver.z3context.bv_val(constant->value.str().c_str(), bits->size);
        return false;
    }

    if (const auto *bits = constant->type->to<IR::Extracted_Varbits>()) {
        result = solver.z3context.bv_val(constant->value.str().c_str(), bits->width_bits());
        return false;
    }

    BUG("Z3Translator: unsupported type for constant %1%", constant);
}

bool Z3Translator::preorder(const IR::BoolLiteral *boolLiteral) {
    result = solver.z3context.bool_val(boolLiteral->value);
    return false;
}

bool Z3Translator::preorder(const IR::Member *member) {
    result = solver.declareVar(member);
    return false;
}

bool Z3Translator::preorder(const IR::ConcolicVariable *var) {
    /// Declare the member contained within the concolic variable
    result = solver.declareVar(var->concolicMember);
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

}  // namespace P4Tools
