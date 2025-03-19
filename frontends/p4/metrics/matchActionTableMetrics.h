#ifndef FRONTENDS_P4_MATCH_ACTION_TABLE_METRICS_H_
#define FRONTENDS_P4_MATCH_ACTION_TABLE_METRICS_H_

#include "../ir/ir.h"
#include "metricsStructure.h"

namespace P4 {

class MatchActionTableMetricsPass : public Inspector {
 private:
    unsigned keySize(const IR::KeyElement *keyElement);
    Metrics &metrics;
 public:
    explicit MatchActionTableMetricsPass(Metrics &metricsRef)
        : metrics(metricsRef) { setName("MatchActionTableMetricsPass"); }
    /// Collect metrics for each match-action table
    bool preorder(const IR::P4Table *table) override;
    /// Calculate averages at the end of traversal
    void postorder([[maybe_unused]] const IR::P4Program *program) override;

};

}  // namespace P4

#endif /* FRONTENDS_P4_MATCH_ACTION_TABLE_METRICS_H_ */
