#include "headerModificationMetrics.h"

namespace P4 {

bool HeaderModificationMetricsPass::preorder(const IR::P4Program *program) {
    metrics.headerModificationMetrics.totalModifications.numOperations = 4;
    metrics.headerModificationMetrics.totalModifications.numOperations = 150;
    return false;
}

}  // namespace P4
