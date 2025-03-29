#include "halsteadMetrics.h"

namespace P4 {

// Helper functions.

void HalsteadMetricsPass::addOperand(const std::string &operand) {
    if (scopedOperands.empty()) {
        scopedOperands.emplace_back();  // Start global scope if none
    }
    if (structFields.find(operand) != structFields.end()){
        uniqueFields.insert(operand);
    }
    else if (scopedOperands.back().insert(operand).second) {
        uniqueOperands.insert(operand);
    }
    metrics.halsteadMetrics.totalOperands++;
}

void HalsteadMetricsPass::addUnaryOperator(const std::string &op) {
    uniqueUnaryOperators.insert(op);
    metrics.halsteadMetrics.totalOperators++;
}

void HalsteadMetricsPass::addBinaryOperator(const std::string &op) {
    uniqueBinaryOperators.insert(op);
    metrics.halsteadMetrics.totalOperators++;
}

// Scope handling.

bool HalsteadMetricsPass::preorder(const IR::P4Control* /*control*/) {
    scopedOperands.emplace_back();
    return true;
}

void HalsteadMetricsPass::postorder(const IR::P4Control* /*control*/) {
    scopedOperands.pop_back();
}

bool HalsteadMetricsPass::preorder(const IR::P4Parser* /*parser*/) {
    scopedOperands.emplace_back();
    return true;
}

void HalsteadMetricsPass::postorder(const IR::P4Parser* /*parser*/) {
    scopedOperands.pop_back();
}

bool HalsteadMetricsPass::preorder(const IR::ActionFunction* /*action*/) {
    scopedOperands.emplace_back();
    return true;
}

void HalsteadMetricsPass::postorder(const IR::ActionFunction* /*action*/) {
    scopedOperands.pop_back();
}

// Collect structure fields to prevent duplicate counting of "unique" operands.

bool HalsteadMetricsPass::preorder(const IR::Type_Header *headerType) {
    if (!headerType) return false;
    for (auto field : headerType->fields) {
        std::string fieldName = field->name.toString().string();
        structFields.insert(fieldName);
    }
    return true;
}

bool HalsteadMetricsPass::preorder(const IR::Type_Struct *structType) {
    if (!structType) return false;
    for (auto field : structType->fields) {
        std::string fieldName = field->name.toString().string();
        structFields.insert(fieldName);
    }
    return true;
}

// Collect operators and operands

bool HalsteadMetricsPass::preorder(const IR::MethodCallExpression *methodCall) {
    if (!methodCall) return false;
    auto methodExpr = methodCall->method;
    std::string method = methodExpr->toString().string();

    if (auto memberExpr = methodExpr->to<IR::Member>()) {
        auto memberName = memberExpr->member.name.string();
        visit(memberExpr);
        addUnaryOperator(memberName);
    }
    else if (auto pathExpr = methodExpr->to<IR::PathExpression>()) {
        auto pathName = pathExpr->path->name.name.string();
        addUnaryOperator(pathName);
    }
    else if (auto nestedMethodCall = methodExpr->to<IR::MethodCallExpression>()) {
        visit(nestedMethodCall);
    }
    visit(methodCall->arguments);
    return false;
}

bool HalsteadMetricsPass::preorder(const IR::Member *member) {
    if (!member) return false;
    std::string fieldName = member->member.name.string();
    // Only add the member if it is not a method call
    if (specialMethods.find(fieldName) == specialMethods.end()){
        addOperand(fieldName);
    }
    addBinaryOperator(".");
    return true;
}

bool HalsteadMetricsPass::preorder(const IR::PathExpression *pathExpr) {
    if (!pathExpr) return false;
    std::string name = pathExpr->path->name.name.string();
    if (matchTypes.find(name) == matchTypes.end()) {
        addOperand(name);
    }
    return true;
}

bool HalsteadMetricsPass::preorder(const IR::Constant *constant) {
    if (!constant) return false;
    std::string valueStr = constant->toString().string();
    addOperand(valueStr);
    return true;
}

bool HalsteadMetricsPass::preorder(const IR::ConstructorCallExpression *ctorCall) {
    if (!ctorCall) return false;
    std::string ctorName = ctorCall->constructedType->toString().string();
    addUnaryOperator("construct:" + ctorName);
    return true;
}

bool HalsteadMetricsPass::preorder(const IR::AssignmentStatement *stmt) {
    if (!stmt) return false;
    addBinaryOperator("=");
    return true;
}

bool HalsteadMetricsPass::preorder(const IR::IfStatement *stmt) {
    if (!stmt) return false;
    addUnaryOperator("if");
    if (stmt->ifFalse != nullptr) {
        addUnaryOperator("else");
    }
    return true;
}

bool HalsteadMetricsPass::preorder(const IR::SwitchStatement *stmt) {
    if (!stmt) return false;
    addUnaryOperator("switch");
    for(size_t i = 0; i < stmt->cases.size(); i++)
        addUnaryOperator("case");

    return true;
}

bool HalsteadMetricsPass::preorder(const IR::ReturnStatement* /*stmt*/) {
    addUnaryOperator("return");
    return true;
}

bool HalsteadMetricsPass::preorder(const IR::ExitStatement* /*stmt*/) {
    addUnaryOperator("exit");
    return true;
}

bool HalsteadMetricsPass::preorder(const IR::Operation_Unary *op) {
    if (!op) return false;
    std::string opName = op->getStringOp().string();
    addUnaryOperator(opName);
    return true;
}

bool HalsteadMetricsPass::preorder(const IR::Operation_Binary *op) {
    if (!op) return false;
    std::string opName = op->getStringOp().string();
    addBinaryOperator(opName);
    return true;
}

bool HalsteadMetricsPass::preorder(const IR::SelectExpression *selectExpr) {
    if (!selectExpr) return false;
    addUnaryOperator("transition");
    addUnaryOperator("select");
    return true; 
}

bool HalsteadMetricsPass::preorder(const IR::SelectCase *caseItem) {
    if (!caseItem) return false;
    addOperand(caseItem->keyset->toString().string());
    if (auto pathExpr = caseItem->state->to<IR::PathExpression>()) {
        std::string nextState = pathExpr->path->name.toString().string();
        addUnaryOperator(nextState);
    }
    return false;
}

bool HalsteadMetricsPass::preorder(const IR::P4Table *table) {
    if (!table) return false;
    for (const auto &property : table->properties->properties) {
        std::string propName = property->name.toString().string();
        addBinaryOperator("=");
        addOperand(propName);
    }
    return true;
}

// Final metrics calculation.

void HalsteadMetricsPass::postorder(const IR::P4Program* /*program*/) {
    metrics.halsteadMetrics.uniqueOperators = uniqueUnaryOperators.size() + uniqueBinaryOperators.size();
    metrics.halsteadMetrics.uniqueOperands = uniqueOperands.size() + uniqueFields.size();

    metrics.halsteadMetrics.vocabulary = metrics.halsteadMetrics.uniqueOperators + metrics.halsteadMetrics.uniqueOperands;
    metrics.halsteadMetrics.length = metrics.halsteadMetrics.totalOperators + metrics.halsteadMetrics.totalOperands;

    metrics.halsteadMetrics.difficulty = metrics.halsteadMetrics.uniqueOperands == 0 ? 0 :
        (metrics.halsteadMetrics.uniqueOperators / 2.0) * (static_cast<double>(metrics.halsteadMetrics.totalOperands) / metrics.halsteadMetrics.uniqueOperands);

    metrics.halsteadMetrics.volume = metrics.halsteadMetrics.vocabulary == 0 ? 0 :
        metrics.halsteadMetrics.length * std::log2(metrics.halsteadMetrics.vocabulary);

    metrics.halsteadMetrics.effort = metrics.halsteadMetrics.difficulty * metrics.halsteadMetrics.volume;
    metrics.halsteadMetrics.deliveredBugs = metrics.halsteadMetrics.volume / 3000.0;

    if(LOGGING(3)){
        std::cout << "Unique Unary Operators:\n";
        for (const auto& op : uniqueUnaryOperators) {
            std::cout << op << "\n";
        }
        std::cout << "\nUnique Binary Operators:\n";
        for (const auto& op : uniqueBinaryOperators) {
            std::cout << op << "\n";
        }
        std::cout << "\nUnique Operands (Vars/Consts):\n";
        for (const auto& operand : uniqueOperands) {
            std::cout << operand << "\n";
        }
        std::cout << "\nUnique Fields :\n";
        for (const auto& operand : uniqueFields) {
            std::cout << operand << "\n";
        }
    }
}

}  // namespace P4
