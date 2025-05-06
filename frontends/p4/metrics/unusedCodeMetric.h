/*
Counts the number of unused code instances by collecting data
before, and after frontend optimalizations. In the first run,
the instance counts, as well as variable and action names are
saved into helper variables in the metrics structure. During
the second run, the unused code counts are determined by
comparing collected data from the previous run with the new data.
*/

#ifndef FRONTENDS_P4_METRICS_UNUSEDCODEMETRIC_H_
#define FRONTENDS_P4_METRICS_UNUSEDCODEMETRIC_H_

#include <algorithm>
#include <string>
#include <vector>

#include "../ir/ir.h"
#include "../lib/log.h"
#include "metricsStructure.h"

using namespace P4::literals;

namespace P4 {

class UnusedCodeMetricPass : public Inspector {
 private:
    Metrics &metrics;
    UnusedCodeInstances currentInstancesCount;
    std::vector<cstring> scope;
    bool isBefore;

    void recordBefore();
    void recordAfter();

 public:
    explicit UnusedCodeMetricPass(Metrics &metrics, bool isBefore)
        : metrics(metrics), isBefore(isBefore) {
        setName("UnusedCodeMetricPass");
    }

    // Scope handling.

    bool scope_enter(cstring name);
    void scope_leave();
    bool preorder(const IR::P4Control *control) override;
    bool preorder(const IR::P4Parser *parser) override;
    bool preorder(const IR::Function *function) override;
    bool preorder(const IR::IfStatement *stmt) override;
    bool preorder(const IR::SwitchCase *caseNode) override;
    void postorder(const IR::P4Control * /*control*/) override;
    void postorder(const IR::P4Parser * /*parser*/) override;
    void postorder(const IR::Function * /*function*/) override;
    void postorder(const IR::IfStatement *stmt) override;
    void postorder(const IR::SwitchCase * /*case*/) override;

    // Data collection.

    bool preorder(const IR::P4Action *action) override;
    void postorder(const IR::ParserState * /*state*/) override;
    void postorder(const IR::Declaration_Variable *node) override;
    void postorder(const IR::Type_Enum * /*node*/) override;
    void postorder(const IR::Type_SerEnum * /*node*/) override;
    void postorder(const IR::Parameter * /*node*/) override;
    void postorder(const IR::ReturnStatement * /*node*/) override;
    void postorder(const IR::P4Program * /*node*/) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_METRICS_UNUSEDCODEMETRIC_H_ */
