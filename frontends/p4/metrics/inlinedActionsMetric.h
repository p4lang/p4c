#ifndef FRONTENDS_P4_INLINED_ACTIONS_H_
#define FRONTENDS_P4_INLINED_ACTIONS_H_

#include "../ir/ir.h"
#include "metricsStructure.h"

namespace P4 {

class InlinedActionsMetricPass : public Inspector {
    private:
    Metrics &metrics;
    std::unordered_set<std::string> actions; 

    public:
    explicit InlinedActionsMetricPass(Metrics &metricsRef)
        : metrics(metricsRef) { 
        setName("InlinedActionsMetricPass"); 
    }
    
    void postorder(const IR::BlockStatement* block) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_INLINED_ACTIONS_H_ */