#ifndef FRONTENDS_P4_HEADER_MANIPULATION_METRICS_H_
#define FRONTENDS_P4_HEADER_MANIPULATION_METRICS_H_

#include "../ir/ir.h"
#include "metricsStructure.h"

namespace P4 {

class HeaderManipulationMetricsPass : public Inspector {
 private:
    Metrics &metrics;
 public:
    explicit HeaderManipulationMetricsPass(Metrics &metricsRef)
        : metrics(metricsRef) { setName("HeaderManipulationMetricsPass"); }
    bool preorder(const IR::P4Program *program) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_HEADER_MANIPULATION_METRICS_H_ */
