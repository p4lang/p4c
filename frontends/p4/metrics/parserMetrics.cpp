#include "parserMetrics.h"

namespace P4 {

bool ParserMetricsPass::preorder(const IR::P4Program *program) {
    metrics.parserMetrics.totalStates = 10;
    return false;
}

}  // namespace P4
