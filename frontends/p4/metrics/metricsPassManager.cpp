#include "metricsPassManager.h"

namespace P4 {

MetricsPassManager::MetricsPassManager(const CompilerOptions &options, TypeMap *typeMap,
                                       Metrics &metricsRef)
    : selectedMetrics(options.selectedMetrics), typeMap(typeMap), metrics(metricsRef) {
    size_t pos = options.file.string().rfind('.');
    fileName =
        (pos != std::string::npos ? options.file.string().substr(0, pos) : options.file.string());

    isolatedFileName = fileName.rfind('/') == std::string::npos
        ? fileName
        : fileName.substr(fileName.rfind('/') + 1);
    isolatedFileName += ".p4";
}

void MetricsPassManager::addInlined(PassManager &pm) {
    if (selectedMetrics.find("inlined"_cs) != selectedMetrics.end() ||
        selectedMetrics.find("unused-code"_cs) != selectedMetrics.end()) {
        // This pass is necessary for determining the correct unused code values for actions.
        pm.addPasses({new InlinedActionsMetricPass(metrics)});
    }
}

void MetricsPassManager::addUnusedCode(PassManager &pm, bool isBefore) {
    if (selectedMetrics.count("unused-code"_cs)) {
        pm.addPasses({new UnusedCodeMetricPass(metrics, isBefore)});
    }
}

void MetricsPassManager::addMetricPasses(PassManager &pm) {
    if (selectedMetrics.count("loc"_cs))
        pm.addPasses({new LinesOfCodeMetricPass(metrics, isolatedFileName)});
    if (selectedMetrics.count("cyclomatic"_cs))
        pm.addPasses({new CyclomaticComplexityPass(metrics)});
    if (selectedMetrics.count("halstead"_cs)) pm.addPasses({new HalsteadMetricsPass(metrics)});
    if (selectedMetrics.count("nesting-depth"_cs))
        pm.addPasses({new NestingDepthMetricPass(metrics)});
    if (selectedMetrics.count("header-general"_cs))
        pm.addPasses({new HeaderMetricsPass(typeMap, metrics)});
    if (selectedMetrics.count("match-action"_cs))
        pm.addPasses({new MatchActionTableMetricsPass(typeMap, metrics)});
    if (selectedMetrics.count("parser"_cs)) pm.addPasses({new ParserMetricsPass(metrics)});
    if (selectedMetrics.count("extern"_cs)) pm.addPasses({new ExternalObjectsMetricPass(metrics)});

    if (selectedMetrics.find("header-manipulation"_cs) != selectedMetrics.end() ||
        selectedMetrics.find("header-modification"_cs) != selectedMetrics.end()) {
        pm.addPasses({new HeaderPacketMetricsPass(typeMap, metrics)});
    }

    if (!selectedMetrics.empty()) {
        pm.addPasses({new ExportMetricsPass(fileName + "_metrics", selectedMetrics, metrics)});
    }
}
}  // namespace P4
