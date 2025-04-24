#ifndef FRONTENDS_P4_MATCH_ACTION_TABLE_METRICS_H_
#define FRONTENDS_P4_MATCH_ACTION_TABLE_METRICS_H_

#include "../ir/ir.h"
#include "metricsStructure.h"
#include "../typeChecking/typeChecker.h"

namespace P4 {

class MatchActionTableMetricsPass : public Inspector {
 private:
    unsigned keySize(const IR::KeyElement *keyElement);
    TypeMap* typeMap;
    Metrics &metrics;
    
 public:
    explicit MatchActionTableMetricsPass(TypeMap* map, Metrics &metricsRef)
        : typeMap(map), metrics(metricsRef) { setName("MatchActionTableMetricsPass"); }
    /// Collect metrics for each match-action table
    void postorder(const IR::P4Table *table) override;
    /// Calculate averages at the end of traversal
    void postorder(const IR::P4Program* /*program*/) override;

};

}  // namespace P4

#endif /* FRONTENDS_P4_MATCH_ACTION_TABLE_METRICS_H_ */
