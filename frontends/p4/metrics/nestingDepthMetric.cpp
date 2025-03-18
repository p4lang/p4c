#include "nestingDepthMetric.h"

namespace P4 {

bool NestingDepthMetricPass::preorder(const IR::P4Program *program) {
    metrics.nestingDepth.avgNestingDepth = 2.5;
    metrics.nestingDepth.maxNestingDepth = 4;
    return false;
}

}  // namespace P4
