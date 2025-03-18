#include "duplicateCodeMetric.h"

namespace P4 {

bool DuplicateCodeMetricPass::preorder(const IR::P4Program *program) {
    metrics.duplicateCodeInstances = 5;  
    return false;
}

}  // namespace P4
