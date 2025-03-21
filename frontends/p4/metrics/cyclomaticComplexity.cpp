#include "cyclomaticComplexity.h"
#include <iostream>

namespace {

class CyclomaticComplexityCalculator : public P4::Inspector {
    int cc;
 public:
    CyclomaticComplexityCalculator() : cc(1) { setName("CyclomaticComplexityCalculator"); }
    int getComplexity() const { return cc; }

    bool preorder([[maybe_unused]] const P4::IR::IfStatement* stmt) override {
        ++cc;
        return true;
    }

    bool preorder(const P4::IR::SwitchStatement* stmt) override {
        cc += stmt->cases.size();
        return true;
    }

    bool preorder(const P4::IR::SelectExpression* selectExpr) override {
        cc += selectExpr->selectCases.size();
        return true;
    }

    bool preorder(const P4::IR::MethodCallExpression* mce) override {
        if (auto pathExpr = mce->method->to<P4::IR::PathExpression>()) {
            if (pathExpr->path->name.name.string() == "verify")
                ++cc;
        }
        return true;
    }

    bool preorder(const P4::IR::P4Table* table) override {
        if (!table) return false;
        auto actionList = table->getActionList();
        cc += actionList->actionList.size();
        return true;
    }
};

}  // anonymous namespace

namespace P4 {

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
