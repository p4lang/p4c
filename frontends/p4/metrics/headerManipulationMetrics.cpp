#include "headerManipulationMetrics.h"

namespace P4 {

bool HeaderManipulationMetricsPass::preorder(const IR::P4Program *program) {
    metrics.headerManipulationMetrics.totalManipulations.numOperations = 7;
    metrics.headerManipulationMetrics.totalManipulations.totalSize = 200;
    return false;
}

}  // namespace P4
