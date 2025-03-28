#include "cyclomaticComplexity.h"
#include <iostream>

namespace P4 {

bool CyclomaticComplexityCalculator::preorder([[maybe_unused]] const IR::IfStatement* stmt) {
    ++cc;
    return true;
}

bool CyclomaticComplexityCalculator::preorder(const IR::SwitchStatement* stmt) {
    cc += stmt->cases.size();
    return true;
}

bool CyclomaticComplexityCalculator::preorder(const IR::SelectExpression* selectExpr) {
    cc += selectExpr->selectCases.size();
    return true;
}

bool CyclomaticComplexityCalculator::preorder(const IR::MethodCallExpression* mce) {
    if (auto pathExpr = mce->method->to<IR::PathExpression>()) {
        if (pathExpr->path->name.name.string() == "verify")
            ++cc;
    }
    return true;
}

bool CyclomaticComplexityCalculator::preorder(const IR::P4Table* table) {
    if (!table) return false;
    auto actionList = table->getActionList();
    cc += actionList->actionList.size();
    return true;
}

bool CyclomaticComplexityPass::preorder(const IR::P4Control *control) {
    CyclomaticComplexityCalculator calculator;
    control->apply(calculator);
    metrics.cyclomaticComplexity[control->name.name.string()] = calculator.getComplexity();
    return false;
}

bool CyclomaticComplexityPass::preorder(const IR::P4Parser *parser) {
    CyclomaticComplexityCalculator calculator;
    parser->apply(calculator);
    metrics.cyclomaticComplexity[parser->name.name.string()] = calculator.getComplexity();
    return false;
}

}  // namespace P4