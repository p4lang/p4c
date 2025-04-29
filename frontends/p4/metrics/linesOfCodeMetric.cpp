#include "linesOfCodeMetric.h"

namespace P4 {

void LinesOfCodeMetricPass::postorder(const IR::Node* node){
    if (!node) return;
    auto si = node->getSourceInfo();
    // Only count lines inside of the compiled program.
    if (!si.isValid() || si.getSourceFile().string().find(sourceFile) == std::string::npos) return;

    for (unsigned L = si.getStart().getLineNumber(); L <= si.getEnd().getLineNumber(); ++L)
        lines.insert(L);
}

void LinesOfCodeMetricPass::postorder(const IR::P4Program* /*program*/){
    metrics.linesOfCode = lines.size();
}

}  // namespace P4
