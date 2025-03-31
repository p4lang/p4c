#ifndef FRONTENDS_P4_DUPLICATE_CODE_H_
#define FRONTENDS_P4_DUPLICATE_CODE_H_

#include "../ir/ir.h"
#include "metricsStructure.h"

namespace P4 {

class DuplicateCodeMetricPass : public Inspector {
 private:
    Metrics &metrics;
 public:
    explicit DuplicateCodeMetricPass(Metrics &metricsRef)
        : metrics(metricsRef) { setName("DuplicateCodePass"); }
    bool preorder(const IR::P4Program* /*program*/) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_DUPLICATE_CODE_H_ */
