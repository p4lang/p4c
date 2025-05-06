/*
Adds code metric collection passes to the frontend pipeline,
based on the "selectedMetrics" option. If any metrics were
selected by the user, the pass which exports them is added
as well.
*/

#ifndef FRONTENDS_P4_METRICS_METRICSPASSMANAGER_H_
#define FRONTENDS_P4_METRICS_METRICSPASSMANAGER_H_

#include "../../common/options.h"
#include "../ir/ir.h"
#include "cyclomaticComplexity.h"
#include "exportMetrics.h"
#include "externalObjectsMetric.h"
#include "halsteadMetrics.h"
#include "headerMetrics.h"
#include "headerPacketMetrics.h"
#include "inlinedActionsMetric.h"
#include "linesOfCodeMetric.h"
#include "matchActionTableMetrics.h"
#include "metricsStructure.h"
#include "nestingDepthMetric.h"
#include "parserMetrics.h"
#include "unusedCodeMetric.h"

using namespace P4::literals;

namespace P4 {

class PassManager;
class Metrics;

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
