#include "unusedCodeMetric.h"

namespace P4 {

bool UnusedCodeMetricPass::preorder(const IR::P4Program *program) {
    metrics.unusedCodeInstances = 3;  // Mock value
    return false;
}

}  // namespace P4
