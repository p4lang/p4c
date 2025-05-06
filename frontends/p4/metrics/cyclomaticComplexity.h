/*
Determines the cyclomatic complexity values of parsers and
controls. It applies the ccCalculator to each parser and control
instance found during traversal. The ccCalculator determines the
CC value by counting decision point nodes (like if statements).
*/

#ifndef FRONTENDS_P4_METRICS_CYCLOMATICCOMPLEXITY_H_
#define FRONTENDS_P4_METRICS_CYCLOMATICCOMPLEXITY_H_

#include "../ir/ir.h"
#include "metricsStructure.h"

namespace P4 {

class CyclomaticComplexityCalculator : public Inspector {
    int cc;

 public:
    CyclomaticComplexityCalculator() : cc(1) { setName("CyclomaticComplexityCalculator"); }
    int getComplexity() const { return cc; }

    void postorder(const IR::IfStatement * /*stmt*/) override;
    void postorder(const IR::SwitchStatement *stmt) override;
    void postorder(const IR::ForStatement * /*stmt*/) override;
    void postorder(const IR::ForInStatement * /*stmt*/) override;
    void postorder(const IR::SelectExpression *selectExpr) override;
    void postorder(const IR::MethodCallExpression *mce) override;
    bool preorder(const IR::P4Table *table) override;
};

class CyclomaticComplexityPass : public Inspector {
    Metrics &metrics;

 public:
    explicit CyclomaticComplexityPass(Metrics &metricsRef) : metrics(metricsRef) {
        setName("CyclomaticComplexityPass");
    }

    bool preorder(const IR::P4Control *control) override;
    bool preorder(const IR::P4Parser *parser) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_METRICS_CYCLOMATICCOMPLEXITY_H_ */
