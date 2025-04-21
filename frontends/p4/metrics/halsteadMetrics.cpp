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

bool HalsteadMetricsPass::preorder(const IR::Function* /*function*/) {
    scopedOperands.emplace_back();
    return true;
}

void HalsteadMetricsPass::postorder(const IR::Function* /*function*/) {
    scopedOperands.pop_back();
}

// Collect structure fields to prevent duplicate counting of "unique" operands.

void HalsteadMetricsPass::postorder(const IR::Type_Header *headerType) {
    for (auto field : headerType->fields) {
        std::string fieldName = field->name.toString().string();
        structFields.insert(fieldName);
    }
}

void HalsteadMetricsPass::postorder(const IR::Type_Struct *structType) {
    for (auto field : structType->fields) {
        std::string fieldName = field->name.toString().string();
        structFields.insert(fieldName);
    }
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
    if (reservedKeywords.find(fieldName) == reservedKeywords.end()){
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

void HalsteadMetricsPass::postorder(const IR::ConstructorCallExpression *ctorCall) {
    if (ctorCall->constructedType)
        addUnaryOperator("construct:" + ctorCall->constructedType->toString().string());
}

bool HalsteadMetricsPass::preorder(const IR::Operation_Unary *op) {
    if (!op) return false;
    std::string opName = op->getStringOp().string();
    addUnaryOperator(opName);
    return true;
}

void HalsteadMetricsPass::postorder(const IR::ParserState* state) {
    auto expr = state->selectExpression;
    if (!expr) return;
    
    // Transition without select.
    if (auto pe = expr->to<IR::PathExpression>()) {
        addUnaryOperator("transition");
        addOperand(pe->path->name.string());
    }
}

void HalsteadMetricsPass::postorder(const IR::IfStatement *stmt) {
    addUnaryOperator("if");
    if (stmt->ifFalse != nullptr) {
        addUnaryOperator("else");
    }
}

void HalsteadMetricsPass::postorder(const IR::SelectExpression* /*selectExpr*/) {
    addUnaryOperator("transition");
    addUnaryOperator("select");
}

bool HalsteadMetricsPass::preorder(const IR::SelectCase *caseItem) {
    if (!caseItem) return false;
    addOperand(caseItem->keyset->toString().string());
    if (auto pathExpr = caseItem->state->to<IR::PathExpression>()) {
        std::string nextState = pathExpr->path->name.toString().string();
        addOperand(nextState);
    }
    return false;
}

void HalsteadMetricsPass::postorder(const IR::P4Table *table) {
    for (const auto &property : table->properties->properties) {
        std::string propName = property->name.toString().string();
        addBinaryOperator("=");
        addOperand(propName);
    }
}

void HalsteadMetricsPass::postorder(const IR::AssignmentStatement* /*stmt*/) {addBinaryOperator("=");}
void HalsteadMetricsPass::postorder(const IR::SwitchStatement* /*stmt*/) {addUnaryOperator("switch");}
void HalsteadMetricsPass::postorder(const IR::SwitchCase* /*case*/){addUnaryOperator("case");}
void HalsteadMetricsPass::postorder(const IR::ReturnStatement* /*stmt*/) {addUnaryOperator("return");}
void HalsteadMetricsPass::postorder(const IR::ExitStatement* /*stmt*/) {addUnaryOperator("exit");}
void HalsteadMetricsPass::postorder(const IR::Constant *constant) {addOperand(constant->toString().string());}
void HalsteadMetricsPass::postorder(const IR::Operation_Binary *op) {addBinaryOperator(op->getStringOp().string());}

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
        std::cout << "Unique Unary Operators ("<< uniqueUnaryOperators.size() <<"):\n";
        for (const auto& op : uniqueUnaryOperators) {
            std::cout << " - " << op << "\n";
        }
        std::cout << "\nUnique Binary Operators (" << uniqueBinaryOperators.size() << "):\n";
        for (const auto& op : uniqueBinaryOperators) {
            std::cout << " - " << op << "\n";
        }
        std::cout << "\nUnique Operands ("<< uniqueOperands.size() <<"):\n";
        for (const auto& operand : uniqueOperands) {
            std::cout << " - " << operand << "\n";
        }
        std::cout << "\nUnique Fields (" << uniqueFields.size() << "):\n";
        for (const auto& operand : uniqueFields) {
            std::cout << " - " << operand << "\n";
        }
    }
}

}  // namespace P4
