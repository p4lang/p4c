#ifndef FRONTENDS_P4_UNUSED_CODE_H_
#define FRONTENDS_P4_UNUSED_CODE_H_

#include "../ir/ir.h"
#include "metricsStructure.h"

namespace P4 {

class UnusedCodeMetricPass : public Inspector {
 private:
    Metrics &metrics;
 public:
    explicit UnusedCodeMetricPass(Metrics &metricsRef)
        : metrics(metricsRef) { setName("UnusedCodeMetricPass"); }
    bool preorder(const IR::P4Program *program) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_UNUSED_CODE_H_ */
