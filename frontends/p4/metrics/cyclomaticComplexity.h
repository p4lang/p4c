#ifndef FRONTENDS_P4_CYCLOMATIC_COMPLEXITY_H_
#define FRONTENDS_P4_CYCLOMATIC_COMPLEXITY_H_

#include "ir/ir.h"
#include "metricsStructure.h"

namespace P4 {

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
