#ifndef FRONTENDS_P4_HEADER_METRICS_H_
#define FRONTENDS_P4_HEADER_METRICS_H_

#include "../ir/ir.h"
#include "metrics.h"

namespace P4 {

class HeaderMetricsPass : public Inspector {
 public:
    HeaderMetricsPass() { setName("HeaderMetricsPass"); }
    bool preorder(const IR::P4Program *program) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_HEADER_METRICS_H_ */
