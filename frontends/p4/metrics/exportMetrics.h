#ifndef FRONTENDS_P4_EXPORTMETRICS_H_
#define FRONTENDS_P4_EXPORTMETRICS_H_

#include <unordered_set>
#include <string>
#include "../ir/ir.h"
#include "metricsStructure.h"

namespace P4 {

class ExportMetricsPass : public Inspector {
 private:
    std::string filename;
    std::unordered_set<std::string> selectedMetrics;
    Metrics &metrics;

 public:
    explicit ExportMetricsPass(const std::string &filename, std::unordered_set<std::string> selectedMetrics, Metrics &metricsRef)
        : filename(filename), selectedMetrics(selectedMetrics), metrics(metricsRef) { setName("ExportMetricsPass");}
    bool preorder(const IR::P4Program* /*program*/) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_EXPORTMETRICS_H_ */