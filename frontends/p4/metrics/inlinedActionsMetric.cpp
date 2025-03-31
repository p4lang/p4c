#include "inlinedActionsMetric.h"

namespace P4 {

bool InlinedActionsMetricPass::preorder(const IR::BlockStatement* block) {
    if (block->getAnnotation(IR::Annotation::inlinedFromAnnotation)) {
        metrics.inlinedActionsNum++;
    }
    return true;
}

}  // namespace P4
