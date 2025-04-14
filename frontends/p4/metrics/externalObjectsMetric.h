#ifndef FRONTENDS_P4_EXTERNAL_OBJECTS_H_
#define FRONTENDS_P4_EXTERNAL_OBJECTS_H_

#include "../ir/ir.h"
#include "metricsStructure.h"
#include <set>

namespace P4 {

class ExternalObjectsMetricPass : public Inspector {
 private:
    Metrics &metrics;
    std::set<std::string> externFunctions;  // Standalone extern functions.
    std::set<std::string> externTypeNames;  // Type names of extern structures.
    std::map<std::string, std::set<std::string>> externMethods;  // Tracks methods per extern.

 public:
    explicit ExternalObjectsMetricPass(Metrics &metricsRef)
        : metrics(metricsRef) { setName("ExternalObjectsMetricPass"); }

    /// Extern structure declaration.
    void postorder(const IR::Type_Extern* node) override;
    /// Extern structure instance.
    void postorder(const IR::Declaration_Instance* node) override;
    /// Extern structure use.
    void postorder(const IR::Member* node) override;
    /// Extern function declaration.
    void postorder(const IR::Method* node) override;
    /// Extern function call.
    void postorder(const IR::MethodCallExpression* node) override;
    /// Print contents of helper sets to stdout if logging is enabled.
    void postorder(const IR::P4Program* /*node*/) override;
    
};

}  // namespace P4

#endif /* FRONTENDS_P4_EXTERNAL_OBJECTS_H_ */