#ifndef METRICS_PASS_MANAGER_H_
#define METRICS_PASS_MANAGER_H_

#include "../../common/options.h"
#include "../ir/ir.h"
#include "metricsStructure.h"
#include "exportMetrics.h"
#include "cyclomaticComplexity.h"
#include "duplicateCodeMetric.h"
#include "externalObjectsMetric.h"
#include "halsteadMetrics.h"
#include "headerManipulationMetrics.h"
#include "headerMetrics.h"
#include "headerModificationMetrics.h"
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
    const std::unordered_set<std::string> &selectedMetrics;
    TypeMap* typeMap;
    Metrics &metrics;

 public:
    MetricsPassManager(const CompilerOptions &options, TypeMap* typeMap, Metrics &metrics)
      : selectedMetrics(options.selectedMetrics), typeMap(typeMap),metrics(metrics) {}

    void addInlined(PassManager &pm) const;
    void addUnusedCode(PassManager &pm) const;
    void addRemaining(PassManager &pm) const;
    void addExportPass(PassManager &pm) const;
};

}  // namespace P4

#endif  // METRICS_PASS_MANAGER_H_
