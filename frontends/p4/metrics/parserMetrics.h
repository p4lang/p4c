/*
Collects parser metrics by applying the CC calculator to each state,
and collecting the number of states of each encountered parser
*/

#ifndef FRONTENDS_P4_METRICS_PARSERMETRICS_H_
#define FRONTENDS_P4_METRICS_PARSERMETRICS_H_

#include "frontends/p4/metrics/cyclomaticComplexity.h"
#include "frontends/p4/metrics/metricsStructure.h"
#include "ir/ir.h"

namespace P4 {

class ParserMetricsPass : public Inspector {
 private:
    ParserMetrics &metrics;

 public:
    explicit ParserMetricsPass(Metrics &metricsRef) : metrics(metricsRef.parserMetrics) {
        setName("ParserMetricsPass");
    }

    bool preorder(const IR::P4Parser *parser) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_METRICS_PARSERMETRICS_H_ */
