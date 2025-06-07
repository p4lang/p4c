#include "frontends/p4/metrics/cyclomaticComplexity.h"

namespace P4 {

void CyclomaticComplexityCalculator::postorder(const IR::IfStatement * /*stmt*/) { ++cc; }
void CyclomaticComplexityCalculator::postorder(const IR::SwitchStatement *stmt) {
    cc += stmt->cases.size();
}
void CyclomaticComplexityCalculator::postorder(const IR::ForStatement * /*stmt*/) { ++cc; }
void CyclomaticComplexityCalculator::postorder(const IR::ForInStatement * /*stmt*/) { ++cc; }
void CyclomaticComplexityCalculator::postorder(const IR::SelectExpression *selectExpr) {
    cc += selectExpr->selectCases.size();
}

void CyclomaticComplexityCalculator::postorder(const IR::MethodCallExpression *mce) {
    if (auto pathExpr = mce->method->to<IR::PathExpression>()) {
        if (pathExpr->path->name.name == "verify"_cs) ++cc;
    }
}

bool CyclomaticComplexityCalculator::preorder(const IR::P4Table *table) {
    if (!table) return false;
    auto actionList = table->getActionList();
    cc += actionList->actionList.size();
    return false;
}

bool CyclomaticComplexityPass::preorder(const IR::P4Control *control) {
    CyclomaticComplexityCalculator calculator;
    control->apply(calculator);
    metrics.cyclomaticComplexity[control->name.name] = calculator.getComplexity();
    return false;
}

bool CyclomaticComplexityPass::preorder(const IR::P4Parser *parser) {
    CyclomaticComplexityCalculator calculator;
    parser->apply(calculator);
    metrics.cyclomaticComplexity[parser->name.name] = calculator.getComplexity();
    return false;
}

}  // namespace P4
