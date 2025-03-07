#ifndef FRONTENDS_P4_HALSTEAD_METRICS_H_
#define FRONTENDS_P4_HALSTEAD_METRICS_H_

#include "../ir/ir.h"
#include "metrics.h"

namespace P4 {

class HalsteadMetricsPass : public Inspector {
 public:
    HalsteadMetricsPass() { setName("HalsteadMetricsPass"); }
    bool preorder(const IR::P4Program *program) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_HALSTEAD_METRICS_H_ */
