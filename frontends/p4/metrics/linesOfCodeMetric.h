/*
Counts the number of "real" lines of code in the compiled program
by inserting the numbers of lines which contain code into a set,
and retrieving it's size at the end.
*/

#ifndef FRONTENDS_P4_METRICS_LINESOFCODEMETRIC_H_
#define FRONTENDS_P4_METRICS_LINESOFCODEMETRIC_H_

#include <filesystem>
#include <string>
#include <unordered_set>

#include "frontends/p4/metrics/metricsStructure.h"
#include "ir/ir.h"

namespace P4 {

class LinesOfCodeMetricPass : public Inspector {
 private:
    Metrics &metrics;
    std::string sourceFile;
    std::unordered_set<unsigned> lines;

 public:
    explicit LinesOfCodeMetricPass(Metrics &metricsRef, std::filesystem::path sourceFile)
        : metrics(metricsRef), sourceFile(sourceFile.stem().string()) {
        setName("LinesOfCodeMetricPass");
    }

    void postorder(const IR::Node *node) override;
    void postorder(const IR::P4Program * /*program*/) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_METRICS_LINESOFCODEMETRIC_H_ */
