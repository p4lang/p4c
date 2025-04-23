#include "inlinedActionsMetric.h"

namespace P4 {

void InlinedActionsMetricPass::postorder(const IR::BlockStatement* block) {
    if (block->getAnnotation(IR::Annotation::inlinedFromAnnotation)) {
        metrics.inlinedActions++;
    }
}

}  // namespace P4
