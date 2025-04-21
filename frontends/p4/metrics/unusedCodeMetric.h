#ifndef FRONTENDS_P4_UNUSED_CODE_METRICS_H_
#define FRONTENDS_P4_UNUSED_CODE_METRICS_H_

#include <vector>
#include <string>
#include <algorithm>
#include "../ir/ir.h"
#include "metricsStructure.h"
#include "../lib/log.h"

namespace P4 {

class UnusedCodeMetricPass : public Inspector {
 private:
    Metrics& metrics;
    UnusedCodeInstances currentInstancesCount;
    std::vector<std::string> scope;
    bool isBefore;

    void recordBefore();
    void recordAfter();

 public:
    explicit UnusedCodeMetricPass(Metrics& metrics, bool isBefore)
        : metrics(metrics), isBefore(isBefore) 
        {setName("UnusedCodeMetricPass");
    }


    // Scope handling.

    void scope_enter(std::string name);
    void scope_leave();
    bool preorder(const IR::P4Control* control) override;
    void postorder(const IR::P4Control* /*control*/) override;
    bool preorder(const IR::P4Parser* parser) override;
    void postorder(const IR::P4Parser* /*parser*/) override;
    bool preorder(const IR::Function* function) override;
    void postorder(const IR::Function* /*function*/) override;
    bool preorder (const IR::IfStatement* stmt) override;
    void postorder(const IR::IfStatement* stmt) override;
    bool preorder(const IR::SwitchCase* caseNode) override;
    void postorder(const IR::SwitchCase* /*case*/) override;
    
    // Data collection.

    bool preorder(const IR::P4Action* action) override;
    void postorder(const IR::ParserState* /*state*/) override;
    void postorder(const IR::Declaration_Variable* node) override;
    void postorder(const IR::Type_Enum* /*node*/) override;
    void postorder(const IR::Type_SerEnum* /*node*/) override;
    void postorder(const IR::Parameter* /*node*/) override;
    void postorder(const IR::ReturnStatement* /*node*/) override;
    void postorder(const IR::P4Program* /*node*/) override;
};

}  // namespace P4

#endif