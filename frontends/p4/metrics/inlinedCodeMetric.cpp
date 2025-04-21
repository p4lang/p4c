#include "inlinedCodeMetric.h"

namespace P4 {

void InlinedCodeMetricPass::postorder(const IR::BlockStatement* block) {
    if (block->getAnnotation(IR::Annotation::inlinedFromAnnotation)) {
        if (actions) metrics.inlinedCode.actions++;
        else { metrics.inlinedCode.functions++;}
    }
}

void InlinedCodeMetricPass::postorder(const IR::P4Program* /*program*/){
    if (!actions) metrics.inlinedCode.functions -= metrics.inlinedCode.actions;
}

}  // namespace P4
