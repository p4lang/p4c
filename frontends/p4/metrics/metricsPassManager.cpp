#include "metricsPassManager.h"

namespace P4 {

void MetricsPassManager::addInlined(PassManager &pm) const {
    if (selectedMetrics.find("inlined") != selectedMetrics.end() ||
            selectedMetrics.find("unused-code") != selectedMetrics.end()) 
        {
        pm.addPasses({new InlinedActionsMetricPass(metrics)});
    }
}

void MetricsPassManager::addUnusedCode(PassManager &pm, bool isBefore) const {
    if (selectedMetrics.find("unused-code") != selectedMetrics.end()) {
        pm.addPasses({new UnusedCodeMetricPass(metrics, isBefore)});
    }
}

void MetricsPassManager::addRemaining(PassManager &pm) const {
    if (selectedMetrics.find("cyclomatic") != selectedMetrics.end()) {
        pm.addPasses({new CyclomaticComplexityPass(metrics)});
    }
    if (selectedMetrics.find("halstead") != selectedMetrics.end()) {
        pm.addPasses({new HalsteadMetricsPass(metrics)});
    }
    if (selectedMetrics.find("duplicate-code") != selectedMetrics.end()) {
        pm.addPasses({new DuplicateCodeMetricPass(metrics)});
    }
    if (selectedMetrics.find("nesting-depth") != selectedMetrics.end()) {
        pm.addPasses({new NestingDepthMetricPass(metrics)});
    }
    if (selectedMetrics.find("header-general") != selectedMetrics.end()) {
        pm.addPasses({new HeaderMetricsPass(metrics)});
    }
    if (selectedMetrics.find("match-action") != selectedMetrics.end()) {
        pm.addPasses({new MatchActionTableMetricsPass(metrics)});
    }
    if (selectedMetrics.find("parser") != selectedMetrics.end()) {
        pm.addPasses({new ParserMetricsPass(metrics)});
    }
    if (selectedMetrics.find("extern") != selectedMetrics.end()) {
        pm.addPasses({new ExternalObjectsMetricPass(metrics)});
    }
    if (selectedMetrics.find("header-manipulation") != selectedMetrics.end()) {
        pm.addPasses({new HeaderManipulationMetricsPass(typeMap, metrics)});
    }
    if (selectedMetrics.find("header-modification") != selectedMetrics.end()) {
        pm.addPasses({new HeaderModificationMetricsPass(typeMap, metrics)});
    }
}

void MetricsPassManager::addExportPass(PassManager &pm) const {
    if (!selectedMetrics.empty()) {
        pm.addPasses({new ExportMetricsPass("metrics_output.txt", selectedMetrics, metrics)});
    }
}

}  // namespace P4
