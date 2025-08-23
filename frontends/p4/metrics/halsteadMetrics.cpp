#include "frontends/p4/metrics/halsteadMetrics.h"

namespace P4 {

// Helper functions.

void HalsteadMetricsPass::addOperand(const cstring &operand) {
    if (scopedOperands.empty()) scopedOperands.emplace_back();
    if (structFields.find(operand) != structFields.end())
        uniqueFields.insert(operand);
    else if (scopedOperands.back().insert(operand).second)
        uniqueOperands.insert(operand);
    metrics.totalOperands++;
}

void HalsteadMetricsPass::addUnaryOperator(const cstring &op) {
    uniqueUnaryOperators.insert(op);
    metrics.totalOperators++;
}

void HalsteadMetricsPass::addBinaryOperator(const cstring &op) {
    uniqueBinaryOperators.insert(op);
    metrics.totalOperators++;
}

// Scope handling.

bool HalsteadMetricsPass::preorder(const IR::P4Control * /*control*/) {
    scopedOperands.emplace_back();
    return true;
}

void HalsteadMetricsPass::postorder(const IR::P4Control * /*control*/) {
    scopedOperands.pop_back();
}

bool HalsteadMetricsPass::preorder(const IR::P4Parser * /*parser*/) {
    scopedOperands.emplace_back();
    return true;
}

void HalsteadMetricsPass::postorder(const IR::P4Parser * /*parser*/) { scopedOperands.pop_back(); }

bool HalsteadMetricsPass::preorder(const IR::Function * /*function*/) {
    scopedOperands.emplace_back();
    return true;
}

void HalsteadMetricsPass::postorder(const IR::Function * /*function*/) {
    scopedOperands.pop_back();
}

// Collect structure fields to prevent duplicate counting of "unique" operands.

void HalsteadMetricsPass::postorder(const IR::Type_Header *headerType) {
    for (auto field : headerType->fields) {
        cstring fieldName = field->name.toString();
        structFields.insert(fieldName);
    }
}

void HalsteadMetricsPass::postorder(const IR::Type_Struct *structType) {
    for (auto field : structType->fields) {
        cstring fieldName = field->name.toString();
        structFields.insert(fieldName);
    }
}

// Collect operators and operands.

bool HalsteadMetricsPass::preorder(const IR::MethodCallExpression *methodCall) {
    if (!methodCall) return false;
    auto methodExpr = methodCall->method;

    if (auto memberExpr = methodExpr->to<IR::Member>()) {
        auto memberName = memberExpr->member.name;
        visit(memberExpr);
        addUnaryOperator(memberName);
    } else if (auto pathExpr = methodExpr->to<IR::PathExpression>()) {
        auto pathName = pathExpr->path->name.name;
        addUnaryOperator(pathName);
    } else if (auto nestedMethodCall = methodExpr->to<IR::MethodCallExpression>()) {
        visit(nestedMethodCall);
    }
    visit(methodCall->arguments);
    return false;
}

bool HalsteadMetricsPass::preorder(const IR::Member *member) {
    if (!member) return false;
    cstring fieldName = member->member.name;
    // Only add the member if it is not a method call
    if (reservedKeywords.find(fieldName) == reservedKeywords.end()) {
        addOperand(fieldName);
    }
    addBinaryOperator("."_cs);
    return true;
}

bool HalsteadMetricsPass::preorder(const IR::PathExpression *pathExpr) {
    if (!pathExpr) return false;
    cstring name = pathExpr->path->name.name;
    if (matchTypes.find(name) == matchTypes.end()) {
        addOperand(name);
    }
    return true;
}

void HalsteadMetricsPass::postorder(const IR::ConstructorCallExpression *ctorCall) {
    if (ctorCall->constructedType)
        addUnaryOperator("construct:"_cs + ctorCall->constructedType->toString());
}

bool HalsteadMetricsPass::preorder(const IR::Operation_Unary *op) {
    if (!op) return false;
    cstring opName = op->getStringOp();
    addUnaryOperator(opName);
    return true;
}

void HalsteadMetricsPass::postorder(const IR::ParserState *state) {
    auto expr = state->selectExpression;
    if (!expr) return;

    // Transition without select.
    if (auto pe = expr->to<IR::PathExpression>()) {
        addUnaryOperator("transition"_cs);
        addOperand(pe->path->name);
    }
}

void HalsteadMetricsPass::postorder(const IR::IfStatement *stmt) {
    addUnaryOperator("if"_cs);
    if (stmt->ifFalse != nullptr) {
        addUnaryOperator("else"_cs);
    }
}

void HalsteadMetricsPass::postorder(const IR::SelectExpression * /*selectExpr*/) {
    addUnaryOperator("transition"_cs);
    addUnaryOperator("select"_cs);
}

bool HalsteadMetricsPass::preorder(const IR::SelectCase *caseItem) {
    if (!caseItem) return false;
    addOperand(caseItem->keyset->toString());
    if (auto pathExpr = caseItem->state->to<IR::PathExpression>()) {
        cstring nextState = pathExpr->path->name.toString();
        addOperand(nextState);
    }
    return false;
}

void HalsteadMetricsPass::postorder(const IR::P4Table *table) {
    for (const auto &property : table->properties->properties) {
        cstring propName = property->name.toString();
        addBinaryOperator("="_cs);
        addOperand(propName);
    }
}

void HalsteadMetricsPass::postorder(const IR::AssignmentStatement * /*stmt*/) {
    addBinaryOperator("="_cs);
}
void HalsteadMetricsPass::postorder(const IR::SwitchStatement * /*stmt*/) {
    addUnaryOperator("switch"_cs);
}
void HalsteadMetricsPass::postorder(const IR::SwitchCase * /*case*/) {
    addUnaryOperator("case"_cs);
}
void HalsteadMetricsPass::postorder(const IR::ReturnStatement * /*stmt*/) {
    addUnaryOperator("return"_cs);
}
void HalsteadMetricsPass::postorder(const IR::ExitStatement * /*stmt*/) {
    addUnaryOperator("exit"_cs);
}
void HalsteadMetricsPass::postorder(const IR::Constant *constant) {
    addOperand(constant->toString());
}
void HalsteadMetricsPass::postorder(const IR::Operation_Binary *op) {
    addBinaryOperator(op->getStringOp());
}
void HalsteadMetricsPass::postorder(const IR::ForStatement * /*stmt*/) {
    addUnaryOperator("for"_cs);
}
void HalsteadMetricsPass::postorder(const IR::ForInStatement * /*stmt*/) {
    addUnaryOperator("for"_cs);
}

// Final metrics calculation.

void HalsteadMetricsPass::postorder(const IR::P4Program * /*program*/) {
    metrics.uniqueOperators = uniqueUnaryOperators.size() + uniqueBinaryOperators.size();
    metrics.uniqueOperands = uniqueOperands.size() + uniqueFields.size();

    metrics.vocabulary = metrics.uniqueOperators + metrics.uniqueOperands;
    metrics.length = metrics.totalOperators + metrics.totalOperands;

    metrics.difficulty =
        metrics.uniqueOperands == 0
            ? 0
            : (metrics.uniqueOperators / 2.0) *
                  (static_cast<double>(metrics.totalOperands) / metrics.uniqueOperands);

    metrics.volume = metrics.vocabulary == 0 ? 0 : metrics.length * std::log2(metrics.vocabulary);

    metrics.effort = metrics.difficulty * metrics.volume;
    metrics.deliveredBugs = metrics.volume / 3000.0;

    if (LOGGING(3)) {
        std::cout << "Unique Unary Operators (" << uniqueUnaryOperators.size() << "):\n";
        for (const auto &op : uniqueUnaryOperators) {
            std::cout << " - " << op << "\n";
        }
        std::cout << "\nUnique Binary Operators (" << uniqueBinaryOperators.size() << "):\n";
        for (const auto &op : uniqueBinaryOperators) {
            std::cout << " - " << op << "\n";
        }
        std::cout << "\nUnique Operands (" << uniqueOperands.size() << "):\n";
        for (const auto &operand : uniqueOperands) {
            std::cout << " - " << operand << "\n";
        }
        std::cout << "\nUnique Fields (" << uniqueFields.size() << "):\n";
        for (const auto &operand : uniqueFields) {
            std::cout << " - " << operand << "\n";
        }
    }
}

}  // namespace P4
