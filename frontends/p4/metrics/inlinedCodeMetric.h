#ifndef FRONTENDS_P4_INLINED_CODE_H_
#define FRONTENDS_P4_INLINED_CODE_H_

#include "../ir/ir.h"
#include "metricsStructure.h"

namespace P4 {

class InlinedCodeMetricPass : public Inspector {
    private:
    Metrics &metrics;
    size_t count = 0;
    bool actions;

    public:
    explicit InlinedCodeMetricPass(Metrics &metricsRef, bool actions)
        : metrics(metricsRef), actions(actions) { 
        setName("InlinedCodeMetricPass"); 
    }
    
    void postorder(const IR::BlockStatement* block) override;
    void postorder(const IR::P4Program* /*program*/) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_INLINED_CODE_H_ */