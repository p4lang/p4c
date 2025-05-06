/*
Counts the number of external structure and function declarations
and uses. When encountering an extern structure declaration, its
method names are collected into "externFunctions". Encountering
an extern method is counted as an extern structure use and as an extern
function call too.
*/

#ifndef FRONTENDS_P4_METRICS_EXTERNALOBJECTSMETRIC_H_
#define FRONTENDS_P4_METRICS_EXTERNALOBJECTSMETRIC_H_

#include <set>

#include "../ir/ir.h"
#include "metricsStructure.h"

namespace P4 {

class ExternalObjectsMetricPass : public Inspector {
 private:
    ExternMetrics &metrics;
    std::set<std::string> externFunctions;                       // Standalone extern functions.
    std::set<std::string> externTypeNames;                       // Type names of extern structures.
    std::map<std::string, std::set<std::string>> externMethods;  // Tracks methods per extern.

 public:
    explicit ExternalObjectsMetricPass(Metrics &metricsRef) : metrics(metricsRef.externMetrics) {
        setName("ExternalObjectsMetricPass");
    }

    /// Extern structure declaration.
    void postorder(const IR::Type_Extern *node) override;
    /// Extern structure instance.
    void postorder(const IR::Declaration_Instance *node) override;
    /// Extern structure use.
    void postorder(const IR::Member *node) override;
    /// Extern function declaration.
    void postorder(const IR::Method *node) override;
    /// Extern function call.
    void postorder(const IR::MethodCallExpression *node) override;
    /// Print contents of helper sets to stdout if logging is enabled.
    void postorder(const IR::P4Program * /*node*/) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_METRICS_EXTERNALOBJECTSMETRIC_H_ */
