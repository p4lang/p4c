#include "metricsPassManager.h"

namespace P4 {

MetricsPassManager::MetricsPassManager(const CompilerOptions &options, TypeMap* typeMap)
    : options(options), 
    selectedMetrics(options.selectedMetrics), 
    typeMap(typeMap), 
    metrics() {
    
    size_t pos = options.file.string().rfind('.');
    fileName = (pos != std::string::npos 
        ? options.file.string().substr(0, pos)
        : options.file.string());

    isolatedFileName = fileName.rfind('/') == std::string::npos
        ? fileName 
        : fileName.substr(fileName.rfind('/') + 1);
    isolatedFileName += ".p4";
}

void MetricsPassManager::addInlined(PassManager &pm){
    if (selectedMetrics.find("inlined") != selectedMetrics.end() ||
        selectedMetrics.find("unused-code") != selectedMetrics.end()) 
    {
        // This pass is necessary for determining the correct unused code values for actions.
        pm.addPasses({new InlinedActionsMetricPass(metrics)});
    }
}
void MetricsPassManager::addUnusedCode(PassManager &pm, bool isBefore){
    if (selectedMetrics.count("unused-code")) {
        pm.addPasses({new UnusedCodeMetricPass(metrics, isBefore)});
    }
}
void MetricsPassManager::addRemaining(PassManager &pm){
    if (selectedMetrics.count("loc")) {
        pm.addPasses({new LinesOfCodeMetricPass(metrics, isolatedFileName)});
    }
    if (selectedMetrics.count("cyclomatic")) {
        pm.addPasses({new CyclomaticComplexityPass(metrics)});
    }
    if (selectedMetrics.count("halstead")) {
        pm.addPasses({new HalsteadMetricsPass(metrics)});
    }
    if (selectedMetrics.count("nesting-depth")) {
        pm.addPasses({new NestingDepthMetricPass(metrics)});
    }
    if (selectedMetrics.count("header-general")) {
        pm.addPasses({new HeaderMetricsPass(typeMap, metrics)});
    }
    if (selectedMetrics.count("match-action")) {
        pm.addPasses({new MatchActionTableMetricsPass(typeMap, metrics)});
    }
    if (selectedMetrics.count("parser")) {
        pm.addPasses({new ParserMetricsPass(metrics)});
    }
    if (selectedMetrics.count("extern")) {
        pm.addPasses({new ExternalObjectsMetricPass(metrics)});
    }
    if (selectedMetrics.find("header-manipulation") != selectedMetrics.end() || 
        selectedMetrics.find("header-modification") != selectedMetrics.end()) {
        pm.addPasses({new HeaderPacketMetricsPass(typeMap, metrics)});
    }
}

void MetricsPassManager::addExportPass(PassManager &pm){
    if (!selectedMetrics.empty()) {
        pm.addPasses({new ExportMetricsPass(fileName + "_metrics", selectedMetrics, metrics)});
    }
}

}  // namespace P4
