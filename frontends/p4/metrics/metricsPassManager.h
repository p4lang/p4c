/*
Adds code metric collection passes to the frontend pipeline,
based on the "selectedMetrics" option. If any metrics were
selected by the user, the pass which exports them is added
as well.
*/

#ifndef FRONTENDS_P4_METRICS_METRICSPASSMANAGER_H_
#define FRONTENDS_P4_METRICS_METRICSPASSMANAGER_H_

#include "frontends/common/options.h"
#include "frontends/p4/metrics/cyclomaticComplexity.h"
#include "frontends/p4/metrics/exportMetrics.h"
#include "frontends/p4/metrics/externalObjectsMetric.h"
#include "frontends/p4/metrics/halsteadMetrics.h"
#include "frontends/p4/metrics/headerMetrics.h"
#include "frontends/p4/metrics/headerPacketMetrics.h"
#include "frontends/p4/metrics/inlinedActionsMetric.h"
#include "frontends/p4/metrics/linesOfCodeMetric.h"
#include "frontends/p4/metrics/matchActionTableMetrics.h"
#include "frontends/p4/metrics/metricsStructure.h"
#include "frontends/p4/metrics/nestingDepthMetric.h"
#include "frontends/p4/metrics/parserMetrics.h"
#include "frontends/p4/metrics/unusedCodeMetric.h"
#include "ir/ir.h"

using namespace P4::literals;

namespace P4 {

class PassManager;

class MetricsPassManager {
 private:
    const std::set<cstring> &selectedMetrics;
    TypeMap *typeMap;
    Metrics &metrics;
    std::string fileName;
    std::string isolatedFileName;

 public:
    MetricsPassManager(const CompilerOptions &options, TypeMap *typeMap, Metrics &metricsRef);

    Metrics &getMetrics() { return metrics; }
    void addInlined(PassManager &pm);
    void addUnusedCode(PassManager &pm, bool isBefore);
    void addMetricPasses(PassManager &pm);
};

}  // namespace P4

#endif /* FRONTENDS_P4_METRICS_METRICSPASSMANAGER_H_ */
