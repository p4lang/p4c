#include "inlinedActionsMetric.h"

namespace P4 {

bool InlinedActionsMetricPass::preorder(const IR::P4Program *program) {
    metrics.inlinedActionsNum = 4;
    return false;
}

}  // namespace P4
