#ifndef FRONTENDS_P4_HEADER_MODIFICATION_METRICS_H_
#define FRONTENDS_P4_HEADER_MODIFICATION_METRICS_H_

#include "../ir/ir.h"
#include "metrics.h"

namespace P4 {

class HeaderModificationMetricsPass : public Inspector {
 private:
    Metrics &metrics;
 public:
    explicit HeaderModificationMetricsPass(Metrics &metricsRef)
        : metrics(metricsRef) { setName("HeaderModificationMetricsPass"); }
    bool preorder(const IR::P4Program *program) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_HEADER_MODIFICATION_METRICS_H_ */
