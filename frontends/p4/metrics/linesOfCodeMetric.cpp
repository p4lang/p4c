#include "linesOfCodeMetric.h"

namespace P4 {

void LinesOfCodeMetricPass::postorder(const IR::Node* node){
    if (!node) return;
    auto si = node->getSourceInfo();
    // Only count nodes inside the compiled program.
    if (!si.isValid() || si.getSourceFile().string().find(sourceFile) == std::string::npos) return;

    startLine = std::min(node->srcInfo.getStart().getLineNumber(), startLine);
    endLine = std::max(node->srcInfo.getEnd().getLineNumber(), endLine);
}

void LinesOfCodeMetricPass::postorder(const IR::P4Program* /*program*/){
    metrics.linesOfCode = startLine > endLine ? 0 : endLine - startLine + 1;
}

}  // namespace P4
