/*
Collects match-action table metrics by inspecting each table's
properties and extracting the number of actions, as well as unwrapping
each key field and extracting its bit width.
*/

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
    MatchActionTableMetrics &metrics;
    
 public:
    explicit MatchActionTableMetricsPass(TypeMap* map, Metrics &metricsRef)
        : typeMap(map), metrics(metricsRef.matchActionTableMetrics) { setName("MatchActionTableMetricsPass"); }

    void postorder(const IR::P4Table *table) override;
    void postorder(const IR::P4Program* /*program*/) override;

};

}  // namespace P4

#endif /* FRONTENDS_P4_MATCH_ACTION_TABLE_METRICS_H_ */
