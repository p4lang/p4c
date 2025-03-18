#ifndef FRONTENDS_P4_PARSER_METRICS_H_
#define FRONTENDS_P4_PARSER_METRICS_H_

#include "../ir/ir.h"
#include "metrics.h"

namespace P4 {

class ParserMetricsPass : public Inspector {
 private:
    Metrics &metrics;
 public:
    explicit ParserMetricsPass(Metrics &metricsRef)
        : metrics(metricsRef) { setName("ParserMetricsPass"); }
    bool preorder(const IR::P4Program *program) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_PARSER_METRICS_H_ */
