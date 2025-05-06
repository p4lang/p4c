/*
Collects global and per-definition header metrics. For each
header definition, it loops over each field and unwraps
its type, until the field's bit width can be extracted.
*/

#ifndef FRONTENDS_P4_METRICS_HEADERMETRICS_H_
#define FRONTENDS_P4_METRICS_HEADERMETRICS_H_

#include "../ir/ir.h"
#include "../typeMap.h"
#include "metricsStructure.h"

namespace P4 {

class HeaderMetricsPass : public Inspector {
 private:
    TypeMap *typeMap;
    HeaderMetrics &metrics;
    unsigned totalFieldsNum = 0;
    unsigned totalFieldsSize = 0;

 public:
    explicit HeaderMetricsPass(TypeMap *typeMap, Metrics &metricsRef)
        : typeMap(typeMap), metrics(metricsRef.headerMetrics) {
        setName("HeaderMetricsPass");
    }

    void postorder(const IR::Type_Header *header) override;
    void postorder(const IR::P4Program * /*program*/) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_METRICS_HEADERMETRICS_H_ */
