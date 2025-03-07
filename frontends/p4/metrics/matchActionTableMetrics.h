#ifndef FRONTENDS_P4_MATCH_ACTION_TABLE_METRICS_H_
#define FRONTENDS_P4_MATCH_ACTION_TABLE_METRICS_H_

#include "../ir/ir.h"
#include "metrics.h"

namespace P4 {

class MatchActionTableMetricsPass : public Inspector {
 public:
    MatchActionTableMetricsPass() { setName("MatchActionTableMetricsPass"); }
    bool preorder(const IR::P4Program *program) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_MATCH_ACTION_TABLE_METRICS_H_ */
