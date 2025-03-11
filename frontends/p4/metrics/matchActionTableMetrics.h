#ifndef FRONTENDS_P4_MATCH_ACTION_TABLE_METRICS_H_
#define FRONTENDS_P4_MATCH_ACTION_TABLE_METRICS_H_

#include "../ir/ir.h"
#include "metrics.h"

namespace P4 {

class MatchActionTableMetricsPass : public Inspector {
 public:
    MatchActionTableMetricsPass() { setName("MatchActionTableMetricsPass"); }
    /// Collect metrics for each match-action table
    bool preorder(const IR::P4Table *table) override;
    /// Calculate averages at the end of traversal
    void postorder([[maybe_unused]] const IR::P4Program *program) override;
private:
    unsigned keySize(const IR::KeyElement *keyElement);
};

}  // namespace P4

#endif /* FRONTENDS_P4_MATCH_ACTION_TABLE_METRICS_H_ */
