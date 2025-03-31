#ifndef FRONTENDS_P4_UNUSED_CODE_METRICS_H_
#define FRONTENDS_P4_UNUSED_CODE_METRICS_H_

#include <vector>
#include <string>
#include <algorithm>
#include "../ir/ir.h"
#include "metricsStructure.h"

namespace P4 {

class UnusedCodeMetricPass : public Inspector {
 private:
    Metrics& metrics;
    UnusedCodeInstances currentInstancesCount;
    bool isBefore;

    void recordBefore();
    void recordAfter();

 public:
    explicit UnusedCodeMetricPass(Metrics& metrics, bool isBefore)
        : metrics(metrics), isBefore(isBefore) 
        {setName("UnusedCodeMetricPass");
    }
    
    void postorder(const IR::P4Control* node) override;
    void postorder(const IR::P4Parser* node) override;
    void postorder(const IR::P4Table* node) override;
    void postorder(const IR::Declaration_Instance* node) override;
    void postorder(const IR::Declaration_Variable* node) override;
    void postorder(const IR::ParserState* node) override;
    void postorder(const IR::Type_Enum* node) override;
    void postorder(const IR::Type_SerEnum* node) override;
    void postorder(const IR::BlockStatement* node) override;
    void postorder(const IR::IfStatement* node) override;
    void postorder(const IR::SwitchStatement* node) override;
    void postorder(const IR::P4Action* node) override;
    void postorder(const IR::Function* node) override;
    void postorder(const IR::Parameter* node) override;
    void postorder(const IR::ReturnStatement* node) override;
    void postorder(const IR::Operation_Unary* node) override;
    void postorder(const IR::P4Program* node) override;
};

}  // namespace P4

#endif