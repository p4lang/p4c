#ifndef FRONTENDS_P4_METRICS_PASS_MANAGER_H_
#define FRONTENDS_P4_METRICS_PASS_MANAGER_H_

#include "../../common/options.h"
#include "../ir/ir.h"
#include "metricsStructure.h"
#include "exportMetrics.h"
#include "cyclomaticComplexity.h"
#include "externalObjectsMetric.h"
#include "halsteadMetrics.h"
#include "headerMetrics.h"
#include "headerPacketMetrics.h"
#include "inlinedActionsMetric.h"
#include "matchActionTableMetrics.h"
#include "nestingDepthMetric.h"
#include "parserMetrics.h"
#include "unusedCodeMetric.h"

namespace P4 {

class PassManager;
class Metrics;

class MetricsPassManager {
 private:
    const CompilerOptions &options;
    const std::set<std::string> &selectedMetrics;
    TypeMap* typeMap;
    Metrics metrics;

 public:
    MetricsPassManager(const CompilerOptions &options, TypeMap* typeMap)
      : options(options), selectedMetrics(options.selectedMetrics), typeMap(typeMap), metrics() {}
    
    Metrics& getMetrics() { return metrics; }
    void addInlined(PassManager &pm);
    void addUnusedCode(PassManager &pm, bool isBefore);
    void addRemaining(PassManager &pm);
    void addExportPass(PassManager &pm);
};

}  // namespace P4

#endif  // METRICS_PASS_MANAGER_H_
