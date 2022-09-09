#include "backends/p4tools/common/lib/taint.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "backends/p4tools/common/lib/ir.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/null.h"

namespace P4Tools {

const IR::StringLiteral Taint::TAINTED_STRING_LITERAL = IR::StringLiteral(cstring("Taint"));

/// For a given expression, this function computes the intervals of that expression that are
/// tainted. For example, for the expression (8w2 ++ taint<8>), the interval [7;0] will have taint.
/// @param index is used to compute the offset from the original input expression when recursing.
/// For example, an expression such as (8w2 ++ taint<8> ++ taint<8>)[19:12] will compute that the
/// intervals [[4:0]] are tainted.
static void getTaintIntervals(const SymbolicMapType& varMap, const IR::Expression* expr,
                              std::vector<std::pair<int, int>>* taintedRanges, int index) {
    CHECK_NULL(expr);
    if (const auto* member = expr->to<IR::Member>()) {
        if (SymbolicEnv::isSymbolicValue(member)) {
            return;
        }
        expr = varMap.at(member);
    }

    if (const auto* taintExpr = expr->to<IR::TaintExpression>()) {
        taintedRanges->emplace_back(
            std::pair<int, int>{index - 1, index - taintExpr->type->width_bits()});
        return;
    }

    if (const auto* concatExpr = expr->to<IR::Concat>()) {
        const auto* leftExpr = concatExpr->left;
        getTaintIntervals(varMap, leftExpr, taintedRanges, index);
        getTaintIntervals(varMap, concatExpr->right, taintedRanges,
                          index - leftExpr->type->width_bits());
        return;
    }
    if (const auto* slice = expr->to<IR::Slice>()) {
        auto slLeftInt = slice->e1->checkedTo<IR::Constant>()->asInt();
        auto slRightInt = slice->e2->checkedTo<IR::Constant>()->asInt();
        // When we hit a concat, we need to decide whether taint is within the range of the slice.
        // We collect all the taint ranges within the concat.
        std::vector<std::pair<int, int>> taintedSubRanges;
        getTaintIntervals(varMap, slice->e0, &taintedSubRanges, slice->e0->type->width_bits());
        // We iterate through these ranges and check whether the slice is within a tainted
        // range. If this is the case, we return taint.
        for (const auto& taintRange : taintedSubRanges) {
            auto leftTaint = taintRange.first;
            auto rightTaint = taintRange.second;
            if (slRightInt <= leftTaint && rightTaint <= slLeftInt) {
                auto marginLeft = std::min(slLeftInt, leftTaint);
                auto marginRight = std::max(slRightInt, rightTaint);
                auto subTaintLeft = index - 1 - (slLeftInt - marginLeft);
                auto subTaintRight = index - 1 - (slLeftInt - marginRight);
                taintedRanges->emplace_back(std::pair<int, int>{subTaintLeft, subTaintRight});
            }
        }
        return;
    }
    if (const auto* binaryExpr = expr->to<IR::Operation_Binary>()) {
        getTaintIntervals(varMap, binaryExpr->left, taintedRanges, index);
        getTaintIntervals(varMap, binaryExpr->right, taintedRanges, index);
        return;
    }
    if (const auto* unaryExpr = expr->to<IR::Operation_Unary>()) {
        getTaintIntervals(varMap, unaryExpr->expr, taintedRanges, index);
        return;
    }
    if (expr->is<IR::Literal>()) {
        return;
    }
    if (expr->is<IR::DefaultExpression>()) {
        return;
    }
    if (expr->is<IR::ConcolicVariable>()) {
        return;
    }
    BUG("Taint pair collection is unsupported for %1% of type %2%", expr, expr->node_type_name());
}

bool Taint::hasTaint(const SymbolicMapType& varMap, const IR::Expression* expr) {
    if (expr->is<IR::TaintExpression>()) {
        return true;
    }
    if (const auto* member = expr->to<IR::Member>()) {
        if (!SymbolicEnv::isSymbolicValue(member)) {
            return hasTaint(varMap, varMap.at(member));
        }
        return false;
    }
    if (const auto* structExpr = expr->to<IR::StructExpression>()) {
        for (const auto* subExpr : structExpr->components) {
            if (hasTaint(varMap, subExpr->expression)) {
                return true;
            }
        }
        return false;
    }
    if (const auto* listExpr = expr->to<IR::ListExpression>()) {
        for (const auto* subExpr : listExpr->components) {
            if (hasTaint(varMap, subExpr)) {
                return true;
            }
        }
        return false;
    }
    if (const auto* binaryExpr = expr->to<IR::Operation_Binary>()) {
        return hasTaint(varMap, binaryExpr->left) || hasTaint(varMap, binaryExpr->right);
    }
    if (const auto* unaryExpr = expr->to<IR::Operation_Unary>()) {
        return hasTaint(varMap, unaryExpr->expr);
    }
    if (expr->is<IR::Literal>()) {
        return false;
    }
    if (const auto* slice = expr->to<IR::Slice>()) {
        auto slLeftInt = slice->e1->checkedTo<IR::Constant>()->asInt();
        auto slRightInt = slice->e2->checkedTo<IR::Constant>()->asInt();
        // We collect all the taint ranges within the concat.
        std::vector<std::pair<int, int>> taintedRanges;
        getTaintIntervals(varMap, slice->e0, &taintedRanges, slice->e0->type->width_bits());
        // We iterate through these ranges and check whether the slice is within a tainted
        // range. If this is the case, we return taint.
        for (const auto& taintRange : taintedRanges) {
            auto leftTaint = taintRange.first;
            auto rightTaint = taintRange.second;
            if (slRightInt <= leftTaint && rightTaint <= slLeftInt) {
                return true;
            }
        }
        return false;
    }
    if (expr->is<IR::DefaultExpression>()) {
        return false;
    }
    if (expr->is<IR::ConcolicVariable>()) {
        return false;
    }
    BUG("Taint checking is unsupported for %1% of type %2%", expr, expr->node_type_name());
}

class TaintPropagator : public Transform {
    const SymbolicMapType& varMap;

    const IR::Node* postorder(IR::Expression* node) override {
        P4C_UNIMPLEMENTED("Taint transformation not supported for node %1% of type %2%", node,
                          node->node_type_name());
    }

    const IR::Node* postorder(IR::Type* type) override {
        // Types can not have taint, just return them.
        return type;
    }

    const IR::Node* postorder(IR::Literal* lit) override {
        // Literals can also not have taint, just return them.
        return lit;
    }

    const IR::Node* postorder(IR::PathExpression* path) override {
        // Path expressions as part of members can be encountered during the post-order traversal.
        // Ignore them.
        return path;
    }

    const IR::Node* postorder(IR::TaintExpression* expr) override { return expr; }

    const IR::Node* postorder(IR::ConcolicVariable* var) override {
        return new IR::Constant(var->type, IRUtils::getMaxBvVal(var->type));
    }
    const IR::Node* postorder(IR::Operation_Unary* unary_op) override { return unary_op->expr; }

    const IR::Node* postorder(IR::Member* member) override {
        // We do not want to split members.
        return member;
    }

    const IR::Node* postorder(IR::Cast* cast) override {
        if (Taint::hasTaint(varMap, cast->expr)) {
            // Try to cast the taint to whatever type is specified.
            auto* taintClone = cast->expr->clone();
            taintClone->type = cast->destType;
            return taintClone;
        }
        // Otherwise we convert the expression to a constant of the cast type.
        // Ultimately, the value here does not matter.
        return IRUtils::getDefaultValue(cast->destType);
    }

    const IR::Node* postorder(IR::Operation_Binary* bin_op) override {
        if (Taint::hasTaint(varMap, bin_op->right)) {
            return bin_op->right;
        }
        return bin_op->left;
    }

    const IR::Node* postorder(IR::Concat* concat) override { return concat; }

    const IR::Node* postorder(IR::Operation_Ternary* ternary_op) override {
        BUG("Operation ternary %1% of type %2% should not be encountered in the taint propagator.",
            ternary_op, ternary_op->node_type_name());
    }

    const IR::Node* preorder(IR::Slice* slice) override {
        // We assume a bit type here...
        BUG_CHECK(!slice->e0->is<IR::Type_Bits>(),
                  "Expected Type_Bits for the slice expression but received %1%",
                  slice->e0->type->node_type_name());
        auto slLeftInt = slice->e1->checkedTo<IR::Constant>()->asInt();
        auto slRightInt = slice->e2->checkedTo<IR::Constant>()->asInt();
        auto width = 1 + slLeftInt - slRightInt;
        const auto* slice_tb = IRUtils::getBitType(width);
        if (Taint::hasTaint(varMap, slice)) {
            // TODO Make this a helper function. Ideally, taint should be like StateVariable.
            return IRUtils::getTaintExpression(slice_tb);
        }
        // Otherwise we convert the expression to a constant of the sliced type.
        // Ultimately, the value here does not matter.
        return IRUtils::getConstant(slice_tb, 0);
    }

 public:
    explicit TaintPropagator(const SymbolicMapType& varMap) : varMap(varMap) {
        visitDagOnce = false;
    }
};

class MaskBuilder : public Transform {
 private:
    const IR::Node* preorder(IR::Member* member) override {
        // Non-tainted members just return the max value, which corresponds to a mask of all zeroes.
        return IRUtils::getConstant(member->type, IRUtils::getMaxBvVal(member->type));
    }

    const IR::Node* preorder(IR::TaintExpression* taintExpr) override {
        // If the member is tainted, we set the mask to ones corresponding to the width of the
        // value.
        return IRUtils::getDefaultValue(taintExpr->type);
    }

    const IR::Node* preorder(IR::Literal* lit) override {
        // Fill out a literal with zeroes.
        const auto* maxConst = IRUtils::getConstant(lit->type, IRUtils::getMaxBvVal(lit->type));
        // If the literal would have been zero anyway, just return it.
        if (lit->equiv(*maxConst)) {
            return lit;
        }
        return maxConst;
    }

 public:
    MaskBuilder() { visitDagOnce = false; }
};

const IR::Literal* Taint::buildTaintMask(const SymbolicMapType& varMap, const Model* completedModel,
                                         const IR::Expression* programPacket) {
    // First propagate taint and simplify the packet.
    const auto* taintedPacket = programPacket->apply(TaintPropagator(varMap));
    // Then create the mask based on the remaining expressions.
    const auto* mask = taintedPacket->apply(MaskBuilder());
    // Produce the evaluated literal. The hex expression should only have 0 or f.
    return completedModel->evaluate(mask);
}

const IR::Expression* Taint::propagateTaint(const SymbolicMapType& varMap,
                                            const IR::Expression* expr) {
    return expr->apply(TaintPropagator(varMap));
}

const IR::Expression* buildMask(const IR::Expression* expr) { return expr->apply(MaskBuilder()); }

}  // namespace P4Tools
