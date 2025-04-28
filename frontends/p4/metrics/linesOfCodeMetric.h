// linesOfCodeMetric.h
#ifndef FRONTENDS_P4_LINES_OF_CODE_METRIC_H_
#define FRONTENDS_P4_LINES_OF_CODE_METRIC_H_

#include "../ir/ir.h"
#include "metricsStructure.h"
#include <unordered_set>
#include <string>

namespace P4 {

class LinesOfCodeMetricPass : public Inspector {
 private:
    Metrics &metrics;
    std::string sourceFile;
    unsigned startLine = ~0U;
    unsigned endLine = 0;
    std::string first;
    std::string last;

 public:
    explicit LinesOfCodeMetricPass(Metrics &metricsRef, std::string sourceFile)
        : metrics(metricsRef), sourceFile(sourceFile) {
        setName("LinesOfCodeMetricPass");
    }

    void postorder(const IR::Node* node) override;
    void postorder(const IR::P4Program* /*program*/) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_LINES_OF_CODE_METRIC_H_ */
