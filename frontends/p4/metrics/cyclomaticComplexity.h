#ifndef FRONTENDS_P4_CYCLOMATIC_COMPLEXITY_H_
#define FRONTENDS_P4_CYCLOMATIC_COMPLEXITY_H_

#include "../ir/ir.h"
#include "metricsStructure.h"

namespace P4 {

class CyclomaticComplexityCalculator : public Inspector {
    int cc;
 public:
    CyclomaticComplexityCalculator() : cc(1) { setName("CyclomaticComplexityCalculator"); }
    int getComplexity() const { return cc; }

    bool preorder([[maybe_unused]] const IR::IfStatement* stmt) override;
    bool preorder(const IR::SwitchStatement* stmt) override;
    bool preorder(const IR::SelectExpression* selectExpr) override;
    bool preorder(const IR::MethodCallExpression* mce) override;
    bool preorder(const IR::P4Table* table) override;
};

class CyclomaticComplexityPass : public Inspector {
    Metrics &metrics;
 public:
    explicit CyclomaticComplexityPass(Metrics &metricsRef)
        : metrics(metricsRef) { setName("CyclomaticComplexityPass"); }

    bool preorder(const IR::P4Control *control) override;
    bool preorder(const IR::P4Parser *parser) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_CYCLOMATIC_COMPLEXITY_H_ */