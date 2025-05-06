/*
Counts the number of actions inlined during the frontend stage by
counting unique action names inside of blocks with annotantions of type
"Annotation::inlinedFromAnnotation".
*/

#ifndef FRONTENDS_P4_METRICS_INLINEDACTIONSMETRIC_H_
#define FRONTENDS_P4_METRICS_INLINEDACTIONSMETRIC_H_

#include "../ir/ir.h"
#include "metricsStructure.h"

namespace P4 {

class InlinedActionsMetricPass : public Inspector {
 private:
    Metrics &metrics;
    std::unordered_set<cstring> actions;

 public:
    explicit InlinedActionsMetricPass(Metrics &metricsRef) : metrics(metricsRef) {
        setName("InlinedActionsMetricPass");
    }

    void postorder(const IR::BlockStatement *block) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_METRICS_INLINEDACTIONSMETRIC_H_ */
