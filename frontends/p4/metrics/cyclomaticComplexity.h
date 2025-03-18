#ifndef FRONTENDS_P4_CYCLOMATIC_COMPLEXITY_H_
#define FRONTENDS_P4_CYCLOMATIC_COMPLEXITY_H_

#include "ir/ir.h"
#include "metrics.h"

namespace P4 {

class CyclomaticComplexityPass : public Inspector {
 private:
    Metrics &metrics;
 public:
    explicit CyclomaticComplexityPass(Metrics &metricsRef)
        : metrics(metricsRef) {setName("CyclomaticComplexityPass");}
    bool preorder(const IR::P4Program *program) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_CYCLOMATIC_COMPLEXITY_H_ */
