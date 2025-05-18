/*
Exports the collected code metric values into a formatted
text file, and a json file. The new filenames are based on the
compiled program name (programName_metrics.txt/json).
*/

#ifndef FRONTENDS_P4_METRICS_EXPORTMETRICS_H_
#define FRONTENDS_P4_METRICS_EXPORTMETRICS_H_

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>

#include "frontends/p4/metrics/metricsStructure.h"
#include "ir/ir.h"
#include "lib/json.h"

using namespace P4::literals;

namespace P4 {

class ExportMetricsPass : public Inspector {
 private:
    std::filesystem::path filename;
    std::set<cstring> selectedMetrics;
    Metrics &metrics;

 public:
    explicit ExportMetricsPass(const std::filesystem::path &filename,
                               std::set<cstring> selectedMetrics, Metrics &metricsRef)
        : filename(filename), selectedMetrics(selectedMetrics), metrics(metricsRef) {
        setName("ExportMetricsPass");
    }
    bool preorder(const IR::P4Program * /*program*/) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_METRICS_EXPORTMETRICS_H_ */
