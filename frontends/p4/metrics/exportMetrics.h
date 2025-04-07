#ifndef FRONTENDS_P4_EXPORTMETRICS_H_
#define FRONTENDS_P4_EXPORTMETRICS_H_

#include <unordered_set>
#include <string>
#include <iostream>
#include <fstream>
#include "../ir/ir.h"
#include "../lib/json.h"
#include "metricsStructure.h"

namespace P4 {

class ExportMetricsPass : public Inspector {
 private:
    std::string filename;
    std::set<std::string> selectedMetrics;
    Metrics &metrics;
    cstring toCString (const std::string& s) { return cstring(s.c_str()); }

 public:
    explicit ExportMetricsPass(const std::string &filename, std::set<std::string> selectedMetrics, Metrics &metricsRef)
        : filename(filename), selectedMetrics(selectedMetrics), metrics(metricsRef) { setName("ExportMetricsPass");}
    bool preorder(const IR::P4Program* /*program*/) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_EXPORTMETRICS_H_ */